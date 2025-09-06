#include <kernel.h>
#include <stdint.h>

netdev_t* networkdevices = NULL;

#if __BYTE_ORDER == __LITTLE_ENDIAN

uint16_t flip_short(uint16_t short_int) {
	uint32_t first_byte = *((uint8_t*)(&short_int));
	uint32_t second_byte = *((uint8_t*)(&short_int) + 1);
	return (first_byte << 8) | (second_byte);
}

uint32_t flip_long(uint32_t long_int) {
	uint32_t first_byte = *((uint8_t*)(&long_int));
	uint32_t second_byte = *((uint8_t*)(&long_int) + 1);
	uint32_t third_byte = *((uint8_t*)(&long_int)  + 2);
	uint32_t fourth_byte = *((uint8_t*)(&long_int) + 3);
	return (first_byte << 24) | (second_byte << 16) | (third_byte << 8) | (fourth_byte);
}

uint8_t flip_byte(uint8_t byte, int num_bits) {
	uint8_t t = byte << (8 - num_bits);
	return t | (byte >> num_bits);
}

uint8_t htonb(uint8_t byte, int num_bits) {
	return flip_byte(byte, num_bits);
}

uint8_t ntohb(uint8_t byte, int num_bits) {
	return flip_byte(byte, 8 - num_bits);
}

uint16_t htons(uint16_t hostshort) {
	return flip_short(hostshort);
}

uint32_t htonl(uint32_t hostlong) {
	return flip_long(hostlong);
}

uint16_t ntohs(uint16_t netshort) {
	return flip_short(netshort);
}

uint32_t ntohl(uint32_t netlong) {
	return flip_long(netlong);
}

#else

/* Big endian machines like motorola, DEC, MIPS, Sparc are Big-Endian.
 * Big-endian architectures match the binary layout of network byte order,
 * so on those systems the faff above isnt required. Yay for them!
 */

uint16_t flip_short(uint16_t short_int) {
	return short_int;
}

uint32_t flip_long(uint32_t long_int) {
	return long_int;
}

uint8_t flip_byte(uint8_t byte, int num_bits) {
	return byte;
}

uint8_t htonb(uint8_t byte, int num_bits) {
	return byte;
}

uint8_t ntohb(uint8_t byte, int num_bits) {
	return byte;
}

uint16_t htons(uint16_t hostshort) {
	return hostshort;
}

uint32_t htonl(uint32_t hostlong) {
	return hostlong;
}

uint16_t ntohs(uint16_t netshort) {
	return netshort;
}

uint32_t ntohl(uint32_t netlong) {
	return netlong;
}

#endif

void network_up()
{
	arp_init();
	ip_init();
	tcp_init();
	dhcp_init();
	init_dns();
}

void network_down()
{
}

bool register_network_device(netdev_t* newdev) {
	/* Add the new network device to the start of the list */
	dprintf("Registered network device '%s'\n", newdev->name);
	newdev->next = networkdevices;
	networkdevices = newdev;
	return true;
}

netdev_t* get_active_network_device() {
	/* First in list, most recent to be detected */
	return networkdevices;
}

netdev_t* find_network_device(const char* name) {
	netdev_t* cur = networkdevices;
	for(; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}