#include <kernel.h>
#include <hashmap.h>

struct hashmap* prefix_hash = NULL;

int string_compare(const void *a, const void *b, [[maybe_unused]] void *udata) {
	const devname_prefix_t* ua = a;
	const devname_prefix_t* ub = b;
	return strcmp(ua->prefix, ub->prefix);
}

uint64_t string_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const devname_prefix_t* a = item;
	return hashmap_sip(a->prefix, strlen(a->prefix), seed0, seed1);
}

bool make_unique_device_name(const char* prefix, char* buffer)
{
	if (prefix == NULL || buffer == NULL) {
		return false;
	}
	int prefix_increment = 0;
	devname_prefix_t find;
	strlcpy(find.prefix, prefix, 15);
	struct devname_prefix_t* exists = hashmap_get(prefix_hash, &find);
	if (exists != NULL) {
		prefix_increment = exists->increment;
		exists->increment++;
	} else {
		find.increment = 1;
		hashmap_set(prefix_hash, &find);
		exists = &find;
	}
	sprintf(buffer, "%s%d", prefix, prefix_increment);
	dprintf("Registered new device name: %s\n", buffer);
	add_random_entropy((uint64_t)buffer); // NOTE: *address* used as entropy!
	hashmap_set(prefix_hash, exists);
	return true;
}

void init_devicenames()
{
	prefix_hash = hashmap_new(sizeof(devname_prefix_t), 0, 5648549036, 225546834, string_hash, string_compare, NULL, NULL);
}
