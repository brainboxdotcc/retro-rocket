#include <kernel.h>

/* IP protocol implementation
 * Reference: https://datatracker.ietf.org/doc/html/rfc791
 */

#define MAX_REASSEMBLIES 64
#define FRAG_TIMEOUT_TICKS 3000 // ~30s at 1000Hz
#define FRAG_GC_INTERVAL 1500 // ~15s at 1000Hz
#define FRAG_MEM_LIMIT (2 * 1024 * 1024)
#define TCP_MAX_PACKET_SIZE (65536 + sizeof(ip_packet_t))

spinlock_t tcp_send_spinlock = 0;
uint16_t last_id;
uint8_t my_ip[4] = {0, 0, 0, 0};
uint8_t zero_hardware_addr[6] = {0, 0, 0, 0, 0, 0};
packet_queue_item_t* packet_queue = NULL;
packet_queue_item_t* packet_queue_end = NULL;
static const char* my_hostname = NULL;
char ip_addr[4] = { 0, 0, 0, 0 };
int is_ip_allocated = 0, is_dns_allocated = 0, is_gateway_allocated = 0, is_mask_allocated = 0;
uint32_t dns_addr = 0, gateway_addr = 0, netmask = 0;
struct hashmap *frag_map = NULL;
static size_t frag_mem_total = 0;

/* Loopback helpers */
static const uint8_t loopback_addr[4] = {127, 0, 0, 1};
static uint8_t ip_scratch_a[TCP_MAX_PACKET_SIZE + 1];
static uint8_t ip_scratch_b[TCP_MAX_PACKET_SIZE + 1];
static int ip_scratch_sel = 0; /* 0 => A, 1 => B */

void ip_handle_packet(ip_packet_t* packet, [[maybe_unused]] int n_len);

static inline bool ip_is_loopback(const uint8_t *ip) {
	return ip[3] == 127; /* 127.0.0.0/8 */
}

void get_ip_str(char* ip_str, const uint8_t* ip) {
	snprintf(ip_str, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

uint32_t str_to_ip(const char* ip_str)
{
	char ip[15];
	char* dot = NULL, *last_dot = ip;
	uint32_t shift = 0, out = 0, dot_count = 0;
	strlcpy(ip, ip_str, 14);
	while ((dot = strchr(last_dot, '.')) != NULL) {
		*dot = 0;
		out |= (atoi(last_dot) << shift);
		shift += 8;
		last_dot = dot + 1;
		dot_count++;
	}
	if (dot_count != 3) {
		return 0;
	}
	out |= (atoi(last_dot) << shift);
	return out;
}

int gethostaddr(unsigned char *addr) {
	memcpy(addr, ip_addr, 4);
	return is_ip_allocated;
}

void sethostaddr(const unsigned char* addr) {
	if (addr[0] == 0 && addr[1] == 0 && addr[2] == 0 && addr[3] == 0) {
		return;
	}
	memcpy(ip_addr, addr, 4);
	is_ip_allocated = 1;
}

void setdnsaddr(uint32_t dns) {
	if (dns == 0) {
		return;
	}
	dns_addr = dns;
	is_dns_allocated = 1;
}

void setgatewayaddr(uint32_t gateway) {
	if (gateway == 0) {
		return;
	}
	gateway_addr = gateway;
	is_gateway_allocated = 1;
}

void sethostname(const char* hostname) {
	if (my_hostname) {
		kfree_null(&my_hostname);
	}
	my_hostname = hostname;
	return;
}

const char* gethostname() {
	return my_hostname ? my_hostname : "retrorocket";
}

uint32_t getdnsaddr() {
	return dns_addr;
}

uint32_t getgatewayaddr() {
	return gateway_addr;
}

void setnetmask(uint32_t nm) {
	if (!nm) {
		return;
	}
	netmask = nm;
	is_mask_allocated = 1;
}

uint32_t getnetmask() {
	return netmask;
}

static void free_frag_list(ip_packet_frag_t *list) {
	ip_packet_frag_t *cur = list;
	while (cur) {
		ip_packet_frag_t *next = cur->next;
		if (cur->packet) {
			kfree_null(&cur->packet);
		}
		kfree_null(&cur);
		cur = next;
	}
}


void ip_frag_gc(void) {
	size_t i = 0;
	void *item;
	uint64_t ticks = get_ticks();

	interrupts_off();
	if (!frag_map) {
		interrupts_on();
		return;
	}
	while (hashmap_iter(frag_map, &i, &item)) {
		ip_fragmented_packet_parts_t *frag = item;

		if ((ticks - frag->last_seen_ticks > FRAG_TIMEOUT_TICKS) ||
		    (frag_mem_total > FRAG_MEM_LIMIT)) {
			// free fragments & adjust memory usage
			free_frag_list(frag->ordered_list);
			frag_mem_total -= frag->size;

			// Important: deleting while iterating invalidates the iterator.
			// The docs say you *must* reset `i=0` after.
			hashmap_delete(frag_map, frag);
			i = 0; // restart iteration
		}
	}
	interrupts_on();
}



uint16_t ip_calculate_checksum(ip_packet_t * packet) {
	// Treat the packet header as a 2-byte-integer array
	// Sum all integers switch to network byte order
	int array_size = sizeof(ip_packet_t) / 2;
	uint16_t * array = (uint16_t*)packet; // XXX: Alignment!
	uint32_t sum = 0;
	for(int i = 0; i < array_size; i++) {
		sum += htons(array[i]);
	}
	uint32_t carry = sum >> 16;
	sum = sum & 0x0000ffff;
	sum = sum + carry;
	uint16_t ret = ~sum;
	return ret;
}

void dequeue_packet(packet_queue_item_t* cur, packet_queue_item_t* last) {
	kfree_null(&cur->packet);
	/* Remove queue entry */
	if (cur == packet_queue) {
		packet_queue = cur->next;
		if (packet_queue == NULL) {
			packet_queue_end = NULL;
		}
	} else if (cur == packet_queue_end) {
		packet_queue_end = last;
		last->next = NULL;
		if (last == packet_queue) {
			packet_queue_end = packet_queue;
		}
	} else {
		last->next = cur->next;
	}
	kfree_null(&cur);
}

/**
 * @brief 100Hz background task hooked to local APIC timer
 */
void ip_idle()
{
	if (packet_queue) {
		time_t current_time = get_ticks();
		packet_queue_item_t* cur = packet_queue;
		packet_queue_item_t* last = NULL;

		for (; cur; ) {
			/* Save next pointer early, because cur may be freed */
			packet_queue_item_t* next = cur->next;

			/* Here we determine if the packet is destined for the local net or the
			 * internet at large. The calculation is actually very easy, we just AND
			 * the source ip against our network mask, and then do the same to the
			 * destination ip address and if both values match, then this packet is for
			 * the local network. If the values are different, we must redirect the
			 * packet to the gateway router. We don't change the IP in the packet, just
			 * the mac address that it is sent to via ethernet. We set a local variable
			 * called arp_dest for this below, which if the packet is for the local net,
			 * is the IP address of the target machine otherwise is the IP of the
			 * router. This is then used in arp_lookup() to decide which mac address
			 * we send the packet to.
			 *
			 * Note that if we have no gateway address everything is considered local.
			 *
			 * These addresses are all in network byte order - it doesn't matter, so
			 * long as the netmask is too!
			 */
			uint32_t dest_ip = *((uint32_t*)&cur->packet->dst_ip);
			uint32_t source_ip = *((uint32_t*)&cur->packet->src_ip);
			uint32_t gw = getgatewayaddr();
			uint8_t arp_dest[4] = { 0 };
			uint8_t dst_hardware_addr[6] = { 0 };
			bool is_local = (gw == 0 || ((dest_ip & netmask) == (source_ip & netmask)));

			*((uint32_t*)&arp_dest) = is_local ? *((uint32_t*)&cur->packet->dst_ip) : gw;

			if (arp_lookup(dst_hardware_addr, arp_dest)) {
				/* The ARP for this MAC has come back now, we can send the packet! */
				ethernet_send_packet(dst_hardware_addr, (uint8_t*)cur->packet, htons(cur->packet->length), ETHERNET_TYPE_IP);
				dequeue_packet(cur, last);
				/* do not advance last, because cur was freed */
			} else if (is_local && cur->arp_tries < 2 && current_time - cur->last_arp > 10) {
				/* After one second, ARP didn't come back, try it again up to 3 times */
				cur->arp_tries++;
				cur->last_arp = current_time;
				arp_send_packet(zero_hardware_addr, arp_dest);
				last = cur; /* safe to advance last, we kept cur */
			} else if (cur->arp_tries == 3 && current_time - cur->last_arp >= 100) {
				/* 3 ARPs have been tried over 3 seconds, and then we waited another ten.
				 * Packet still didn't get an ARP reply. Dequeue it as a lost packet.
				 */
				dequeue_packet(cur, last);
				/* again, don't advance last */
			} else {
				last = cur; /* still alive, advance last */
			}

			cur = next; /* always advance from saved next pointer */
		}
	}
}

/**
 * @brief Indeterminate frequency foreground task that steals cycles from idle
 */
void ip_foreground() {
	if ((get_ticks() % FRAG_GC_INTERVAL) == 0) {
		ip_frag_gc();
	}
}

void queue_packet([[maybe_unused]] uint8_t* dst_ip, void* data, [[maybe_unused]] uint16_t len) {
	if (packet_queue == NULL) {
		packet_queue = kmalloc(sizeof(packet_queue_item_t));
		if (!packet_queue) {
			return;
		}
		packet_queue_end = packet_queue;
		packet_queue_end->packet = (ip_packet_t*)data;
		packet_queue_end->next = NULL;
		packet_queue_end->last_arp = get_ticks();
		packet_queue_end->arp_tries = 0;
	} else {
		packet_queue_end->next = kmalloc(sizeof(packet_queue_item_t));
		if (!packet_queue_end->next) {
			return;
		}
		packet_queue_end->next->packet = (ip_packet_t*)data;
		packet_queue_end->next->next = NULL;
		packet_queue_end->next->arp_tries = 0;
		packet_queue_end->next->last_arp = get_ticks();
		packet_queue_end = packet_queue_end->next;
	}
}

void ip_send_packet(uint8_t* dst_ip, void* data, uint16_t len, uint8_t protocol) {
	uint64_t flags;
	lock_spinlock_irq(&tcp_send_spinlock, &flags);
	uint8_t dst_hardware_addr[6] = { 0, 0, 0, 0, 0, 0 };
	ip_packet_t *packet = (ip_packet_t *)(ip_scratch_sel ? ip_scratch_b : ip_scratch_a);
	memset(packet, 0, sizeof(ip_packet_t));
	packet->version = IP_IPV4;
	packet->ihl = sizeof(ip_packet_t) / sizeof(uint32_t); // header length is in 32 bit words
	packet->tos.bits = 0; // Don't care
	packet->length = sizeof(ip_packet_t) + len;
	packet->id = last_id++;
	// No fragmentation on outbound packets please! We'll implement this later.
	packet->frag.dont_fragment = 1;
	packet->frag.more_fragments_follow = 0;
	packet->frag.fragment_offset_high = 0;
	packet->frag.fragment_offset_low = 0;
	packet->ttl = 64;
	packet->protocol = protocol;

	gethostaddr(my_ip);
	uint32_t netmask = getnetmask();
	uint32_t our_gateway = getgatewayaddr();
	uint32_t our_ip = *((uint32_t*)&my_ip);
	uint32_t target_ip = *((uint32_t*)dst_ip);
	bool redirected = false;

	memcpy(packet->src_ip, my_ip, 4);
	memcpy(packet->dst_ip, dst_ip, 4);

	/* Force localhost source before checksum if 127/8 */
	if (ip_is_loopback(dst_ip)) {
		memcpy(packet->src_ip, loopback_addr, 4);
	}

	void* packet_data = (void*)packet + packet->ihl * 4;
	memcpy(packet_data, data, len);
	*((uint8_t*)(&packet->version_ihl_ptr)) = htonb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
	*((uint8_t*)(packet->flags_fragment_ptr)) = htonb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
	packet->length = htons(sizeof(ip_packet_t) + len);
	packet->header_checksum = 0;
	packet->header_checksum = htons(ip_calculate_checksum(packet));
	// Attempt to resolve ARP or find in arp cache table

	if (ip_is_loopback(dst_ip)) {
		ip_scratch_sel ^= 1;

		uint16_t pkt_len = ntohs(packet->length);
		unlock_spinlock_irq(&tcp_send_spinlock, flags);

		ip_handle_packet(packet, pkt_len);
		return;
	}

	if (netmask != 0 && our_ip != 0 && target_ip != 0 && ((our_ip & netmask) != (target_ip & netmask))) {
		/* We need to redirect this packet to the router's MAC address */
		if (!arp_lookup(dst_hardware_addr, (uint8_t*)&our_gateway)) {
			ip_packet_t *heap_copy = kmalloc(ntohs(packet->length));
			if (heap_copy) {
				memcpy(heap_copy, packet, ntohs(packet->length));
				queue_packet(dst_ip, heap_copy, heap_copy->length);
			}
			arp_send_packet(zero_hardware_addr, (uint8_t*)&our_gateway);
			unlock_spinlock_irq(&tcp_send_spinlock, flags);
			return;
		}
		redirected = true;
	}

	if (!redirected && !arp_lookup(dst_hardware_addr, dst_ip)) {
		/* Send ARP packet, and add to queue for this mac address */
		ip_packet_t *heap_copy = kmalloc(ntohs(packet->length));
		if (heap_copy) {
			memcpy(heap_copy, packet, ntohs(packet->length));
			queue_packet(dst_ip, heap_copy, heap_copy->length);
		}
		arp_send_packet(zero_hardware_addr, dst_ip);
		unlock_spinlock_irq(&tcp_send_spinlock, flags);
		return;
	}
	ethernet_send_packet(dst_hardware_addr, (uint8_t*)packet, htons(packet->length), ETHERNET_TYPE_IP);
	unlock_spinlock_irq(&tcp_send_spinlock, flags);
}

/**
 * @brief Insert fragmented packet part into ordered list
 * 
 * @param insert fragment to insert (will be sorted by offset)
 * @param list list to insert into
 * @return ip_packet_frag_t* pointer to start of list
 */
ip_packet_frag_t* frag_list_insert(ip_packet_frag_t *insert, ip_packet_frag_t *list)
{
	ip_packet_frag_t *tmp = list, *tmp2;
	if (!tmp) {
		insert->prev = NULL;
		insert->next = NULL;
		return insert;
	}
	if (tmp->offset >= insert->offset) {	
		tmp->prev = insert;
		insert->next = tmp;
		insert->prev = NULL;
		return insert;
	}
	tmp2 = tmp;
	tmp = tmp->next;
	while (tmp) {
		if (tmp->offset >= insert->offset) {
			tmp->prev = insert;
			insert->next = tmp;
			tmp2->next = insert;
			insert->prev = tmp2;
			return list;
		}
		tmp2 = tmp;
		tmp = tmp->next;
	}
	insert->next = NULL;
	tmp2->next = insert;
	insert->prev = tmp2;
	return list;	
}

/**
 * @brief Comparison function for hash table of fragmented packet lists
 * 
 * @param a first object to compare
 * @param b second object to compare
 * @param udata user data
 * @return int 0 for equal, -1 for less than, 1 for greater than; like strcmp()
 */
int ip_frag_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
	const ip_fragmented_packet_parts_t* fa = a;
	const ip_fragmented_packet_parts_t* fb = b;
	return fa->id == fb->id ? 0 : (fa->id < fb->id ? -1 : 1);
}

/**
 * @brief Hash two lists of IP fragments for storage in hashmap, keyed by packet id
 * 
 * @param item item to hash
 * @param seed0 first seed from hashmap
 * @param seed1 second seed from hashmap
 * @return uint64_t hash bucket value
 */
uint64_t ip_frag_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const ip_fragmented_packet_parts_t* frag_parts = item;
	return (uint64_t)frag_parts->id * seed0 * seed1;
}

/**
 * @brief Handle inbound IP packet
 * @note happens in interrupt!
 * 
 * @param packet IP packet to parse
 * @param n_len Packet length
 */
void ip_handle_packet(ip_packet_t* packet, [[maybe_unused]] int n_len) {
	char src_ip[20];
	*((uint8_t*)(&packet->version_ihl_ptr)) = ntohb(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
	*((uint8_t*)(packet->flags_fragment_ptr)) = ntohb(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
	add_random_entropy(packet->header_checksum ^ (*(uint32_t*)packet->src_ip));
	if (packet->version == IP_IPV4) {

		get_ip_str(src_ip, packet->src_ip);
		void * data_ptr = (void*)packet + packet->ihl * 4;
		size_t data_len = ntohs(packet->length) - (packet->ihl * 4);
		bool fragment_to_free = false;

		/* For fragmented packets, we store the fragments in an ordered list,
		 * sorted by fragment offset. Higher offsets overwrite lower ones to
		 * defend against duplicate or malicious fragments.
		 *
		 * Each fragmented set is tracked in a hashmap keyed by the packet ID.
		 * Fragments are appended as they arrive. Once the final fragment is
		 * received, the full packet is reassembled and the entries removed
		 * from the hashmap.
		 *
		 * To prevent resource abuse (e.g. incomplete sets never finishing),
		 * each fragmented set records a last_seen_ticks timestamp. A
		 * background GC routine runs periodically (from the scheduler idle
		 * loop or timer tick) and reaps any sets that have exceeded the
		 * timeout window, releasing their memory safely.
		 *
		 * This closes the long‑standing DoS hole where attackers could leak
		 * memory by sending endless partial fragment sets.
		 */
		uint16_t frag_offset = ((uint16_t)packet->frag.fragment_offset_low | ((uint16_t)packet->frag.fragment_offset_high << 8));
		if (!packet->frag.dont_fragment) {
			if (frag_map == NULL) {
				/* First time we see a fragmented packet, make the hashmap to hold them */
				frag_map = hashmap_new(sizeof(ip_fragmented_packet_parts_t), 0, 564364368549036, 67545346834, ip_frag_hash, ip_frag_compare, NULL, NULL);
			}
			if (packet->frag.more_fragments_follow) {
				/* Packet is part of a fragmented set */
				if (frag_offset == 0) {
					/* First fragment */
					ip_fragmented_packet_parts_t fragmented = { .id = packet->id, .size = data_len, .ordered_list = NULL, .last_seen_ticks = get_ticks() };
					ip_packet_frag_t* fragment = kmalloc(sizeof(ip_packet_frag_t));
					if (!fragment) {
						return;
					}
					fragment->offset = frag_offset;
					fragment->packet = kmalloc(ntohs(packet->length));
					if (!fragment->packet) {
						kfree_null(&fragment);
						return;
					}
					memcpy(fragment->packet, packet, ntohs(packet->length));
					frag_list_insert(fragment, fragmented.ordered_list);
					hashmap_set(frag_map, &fragmented);
				} else {
					/* Middle fragment */
					ip_packet_t findpacket = { .id = packet->id };
					ip_fragmented_packet_parts_t* fragmented = (ip_fragmented_packet_parts_t*)hashmap_get(frag_map, &findpacket);
					if (fragmented == NULL) {
						dprintf("*** WARN *** Fragmented packet id %u has no entry in hash map", packet->id);
						return;
					}
					fragmented->last_seen_ticks = get_ticks();
					ip_packet_frag_t* fragment = kmalloc(sizeof(ip_packet_frag_t));
					if (!fragment) {
						return;
					}
					fragmented->size += data_len;
					fragment->offset = frag_offset;
					fragment->packet = kmalloc(ntohs(packet->length));
					if (!fragment->packet) {
						kfree_null(&fragment);
						return;
					}
					memcpy(fragment->packet, packet, ntohs(packet->length));
					frag_list_insert(fragment, fragmented->ordered_list);
				}
				return;
			} else if (packet->frag.more_fragments_follow == 0 && (frag_offset != 0)) {
				/* Final fragment of fragmented set.
				 * Once we get this fragment, we can deliver the reassembled packet.
				 */
				ip_packet_t findpacket = { .id = packet->id };
				ip_fragmented_packet_parts_t* fragmented = (ip_fragmented_packet_parts_t*)hashmap_get(frag_map, &findpacket);
				if (fragmented == NULL) {
					dprintf("*** WARN *** Fragmented packet id %u has no entry in hash map", packet->id);
					return;
				}
				fragmented->last_seen_ticks = get_ticks();
				ip_packet_frag_t* fragment = kmalloc(sizeof(ip_packet_frag_t));
				if (!fragment) {
					return;
				}
				fragmented->size += data_len;
				fragment->offset = frag_offset;
				fragment->packet = kmalloc(ntohs(packet->length));
				if (!fragment->packet) {
					kfree_null(&fragment->packet);
					return;
				}
				memcpy(fragment->packet, packet, ntohs(packet->length));
				frag_list_insert(fragment, fragmented->ordered_list);

				/* We have complete packet, all fragments - reassemble the data part and free everything */
				ip_packet_frag_t* cur = fragmented->ordered_list;
				data_ptr = kmalloc(fragmented->size);
				if (!data_ptr) {
					dprintf("Can't allocate memory for frag\n");
					return;
				}
				data_len = fragmented->size;
				/* Set flag to indicate we need to free the data_ptr later */
				fragment_to_free = true;

				for (; cur; ) {
					void* next = cur->next;
					size_t this_packet_size = ntohs(cur->packet->length) - (cur->packet->ihl * 4);
					if (cur->offset + this_packet_size < data_len) {
						void * copy_from = (void*)cur->packet + cur->packet->ihl * 4;
						memcpy(data_ptr + cur->offset, copy_from, this_packet_size);
					} else {
						dprintf("*** WARN *** Fragmented packet id %d has fragment with offset %08x and length %08ld >= data length of %08lx", fragmented->id, cur->offset, this_packet_size, data_len);
					}
					kfree_null(&cur->packet);
					kfree_null(&cur);
					cur = next;
				}

				hashmap_delete(frag_map, &findpacket);
				/* Now we have reassembled the data portion, we can fall through and let the packet be handled... */
			}
		}

		if (packet->protocol == PROTOCOL_ICMP) {
			icmp_handle_packet(packet, data_ptr, data_len);
		} else if (packet->protocol == PROTOCOL_UDP) {
			udp_handle_packet(packet, data_ptr, data_len);
		} else if (packet->protocol == PROTOCOL_TCP) {
			tcp_handle_packet(packet, data_ptr, data_len);
		}

		if (fragment_to_free) {
			kfree_null(&data_ptr);
		}
	} else {
		dprintf("Unknown IP packet type %04X\n", packet->version);
	}
}

void ip6_handle_packet([[maybe_unused]] void* packet, [[maybe_unused]] int n_len)
{
}

void ip_init()
{
	ethernet_register_iee802_number(ETHERNET_TYPE_IP, (ethernet_protocol_t)ip_handle_packet);
	ethernet_register_iee802_number(ETHERNET_TYPE_IP6, (ethernet_protocol_t)ip6_handle_packet);
	proc_register_idle(ip_idle, IDLE_BACKGROUND, 1);
	proc_register_idle(ip_foreground, IDLE_FOREGROUND, 1);
}
