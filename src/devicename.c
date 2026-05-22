#include <kernel.h>
#include <hashmap.h>

struct hashmap* prefix_hash = NULL;

int string_compare(const void *a, const void *b, [[maybe_unused]] __attribute__((unused)) void *udata) {
	const devname_prefix_t* ua = a;
	const devname_prefix_t* ub = b;
	return strcmp(ua->prefix, ub->prefix);
}

uint64_t string_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const devname_prefix_t* a = item;
	return hashmap_sip(a->prefix, strlen(a->prefix), seed0, seed1);
}

bool make_unique_device_name(const char* prefix, const char* owner, char* buffer, size_t buffer_len) {
	if (prefix == NULL || owner == NULL || buffer == NULL) {
		return false;
	}
	int prefix_increment = 0;
	devname_prefix_t find = {};
	find.prefix = prefix;
	devname_prefix_t* exists = hashmap_get(prefix_hash, &find);
	if (exists != NULL) {
		prefix_increment = exists->increment;
		exists->increment++;
		if ((size_t)exists->increment > exists->owner_count) {
			const char** new_owners = krealloc(exists->owners, sizeof(const char*) * exists->increment);
			if (!new_owners) {
				return false;
			}
			exists->owners = new_owners;
			exists->owner_count = exists->increment;
		}
		exists->owners[prefix_increment] = owner;
	} else {
		find.prefix = strdup(prefix);
		if (!find.prefix) {
			return false;
		}
		find.increment = 1;
		find.owner_count = 1;
		find.owners = kmalloc(sizeof(const char*));
		if (!find.owners) {
			kfree_null(&find.prefix);
			return false;
		}
		find.owners[0] = owner;
		hashmap_set(prefix_hash, &find);
		exists = hashmap_get(prefix_hash, &find);
		if (!exists) {
			kfree_null(&find.prefix);
			kfree_null(&find.owners);
			return false;
		}
	}
	snprintf(buffer, buffer_len, "%s%d", prefix, prefix_increment);
	dprintf("Registered new device name: %s (%s)\n", buffer, owner);
	add_random_entropy((uint64_t)buffer);
	return true;
}

bool deregister_device_name(const char* prefix, int suffix) {
	if (prefix == NULL || suffix < 0) {
		return false;
	}
	devname_prefix_t find = {};
	find.prefix = prefix;
	devname_prefix_t* exists = hashmap_get(prefix_hash, &find);

	if (exists == NULL) {
		return false;
	}
	if ((size_t)suffix >= exists->owner_count) {
		return false;
	}
	if (exists->owners[suffix] == NULL) {
		return false;
	}
	exists->owners[suffix] = NULL;

	const void* empty[exists->owner_count];
	memset(empty, 0, exists->owner_count * sizeof(void*));

	if (!memcmp(exists->owners, empty, sizeof(empty))) {
		kfree_null(&exists->owners);
		kfree_null(&exists->prefix);
		hashmap_delete(prefix_hash, &find);
	}

	return true;
}

bool get_device_names(devname_entry_t *entries, size_t *count) {
	if (entries == NULL || count == NULL) {
		return false;
	}
	size_t out = 0;
	size_t iter = 0;
	void *item = NULL;
	while (hashmap_iter(prefix_hash, &iter, &item)) {
		devname_prefix_t *prefix = item;
		if (prefix == NULL) {
			continue;
		}
		for (int i = 0; i < prefix->increment; ++i) {
			if (out >= *count) {
				return false;
			}
			entries[out].name = prefix->prefix;
			entries[out].suffix = i;
			entries[out].owner = prefix->owners[i];
			out++;
		}
	}
	*count = out;
	return true;
}

size_t get_device_name_count() {
	size_t count = 0;
	size_t iter = 0;
	void *item = NULL;
	while (hashmap_iter(prefix_hash, &iter, &item)) {
		devname_prefix_t *prefix = item;
		if (prefix == NULL) {
			continue;
		}
		for (int i = 0; i < prefix->increment; ++i) {
			if (prefix->owners[i] != NULL) {
				count++;
			}
		}
	}
	return count;
}

void init_devicenames() {
	prefix_hash = hashmap_new(sizeof(devname_prefix_t), 0, 5648549036, 225546834, string_hash, string_compare, NULL, NULL);
}