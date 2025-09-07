#include <kernel.h>

static struct hashmap* modules;

const void *kernel_dlsym(const char *name) {
	if (!name) {
		return NULL;
	}
	uint64_t a = findsymbol_addr(name);
	return a ? (const void *) (uintptr_t) a : NULL;
}

bool parse_elf_rel_headers(const uint8_t *file, size_t len, const elf_ehdr **eh, const elf_shdr **sh, size_t *shnum, const char **shstr) {
	const elf_ehdr *hdr;

	if (file == NULL || len < sizeof(elf_ehdr)) {
		return false;
	}

	hdr = (const elf_ehdr *) file;

	if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 || hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3) {
		return false;
	} else if (hdr->e_ident[EI_CLASS] != ELFCLASS64) {
		return false;
	} else if (hdr->e_machine != EM_X86_64) {
		return false;
	} else if (hdr->e_type != ET_REL) {
		return false;
	} else if ((size_t) hdr->e_shoff + (size_t) hdr->e_shentsize * (size_t) hdr->e_shnum > len) {
		return false;
	}

	*eh = hdr;
	*sh = (const elf_shdr *) (file + hdr->e_shoff);
	*shnum = hdr->e_shnum;

	const elf_shdr *shstr_hdr = &(*sh)[hdr->e_shstrndx];
	if ((size_t) shstr_hdr->sh_offset >= len) {
		return false;
	}
	*shstr = (const char *) (file + shstr_hdr->sh_offset);

	return true;
}

bool is_alloc_section(const elf_shdr *h) {
	if (h == NULL) {
		return false;
	}
	return (h->sh_flags & SHF_ALLOC) != 0;
}

size_t address_align_up(size_t v, size_t a) {
	size_t m = (a == 0) ? 1 : a;
	size_t r = v % m;

	if (r != 0) {
		return v + (m - r);
	}

	return v;
}

bool module_place_sections(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m) {
	size_t i;
	size_t total = 0;
	size_t max_align = 16;

	if (file == NULL || sh == NULL || m == NULL) {
		return false;
	}

	m->placed_count = 0;
	size_t alloc_secs = 0;

	for (i = 0; i < shnum; i++) {
		const elf_shdr *h = &sh[i];
		size_t a;

		if (!is_alloc_section(h)) {
			continue;
		}

		a = h->sh_addralign ? (size_t) h->sh_addralign : 1;
		if (a > max_align) {
			max_align = a;
		}

		total = address_align_up(total, a);
		total += (size_t) h->sh_size;

		if (++alloc_secs >= (sizeof(m->placed) / sizeof(m->placed[0]))) {
			dprintf("module_place_sections: too many alloc sections\n");
			return false;
		}
	}

	m->size = address_align_up(total, max_align ? max_align : 16);
	m->base = kmalloc_aligned(m->size, max_align > 16 ? max_align : 16);
	if (m->base == NULL) {
		dprintf("module_place_sections: allocation failed (%lu bytes)\n", m->size);
		return false;
	}
	memset(m->base, 0, m->size);

	size_t off = 0;

	for (i = 0; i < shnum; i++) {
		const elf_shdr *h = &sh[i];
		size_t a;

		if (!is_alloc_section(h)) {
			continue;
		}

		a = h->sh_addralign ? (size_t) h->sh_addralign : 1;
		off = address_align_up(off, a);

		m->placed[m->placed_count].shndx = (uint16_t) i;
		m->placed[m->placed_count].addr = m->base + off;
		m->placed_count++;

		if (h->sh_type != SHT_NOBITS && h->sh_size > 0) {
			const uint8_t *src = (const uint8_t *) (file + h->sh_offset);
			memcpy(m->base + off, src, (size_t) h->sh_size);
		}

		off += (size_t) h->sh_size;
	}

	return true;
}

uint8_t *module_section_base(const module *m, uint16_t shndx) {
	if (m == NULL) {
		return NULL;
	}

	for (size_t i = 0; i < m->placed_count; i++) {
		if (m->placed[i].shndx == shndx) {
			return m->placed[i].addr;
		}
	}
	return NULL;
}

bool module_load_symbols(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m) {
	if (file == NULL || sh == NULL || m == NULL) {
		return false;
	}

	m->symtab = NULL;
	m->sym_count = 0;
	m->strtab = NULL;

	for (size_t i = 0; i < shnum; i++) {
		if (sh[i].sh_type == SHT_SYMTAB) {
			m->symtab = (const elf_sym *) (file + sh[i].sh_offset);
			m->sym_count = (size_t) (sh[i].sh_size / sizeof(elf_sym));
			m->strtab = (const char *) (file + sh[sh[i].sh_link].sh_offset);
			break;
		}
	}

	if (m->symtab == NULL || m->strtab == NULL) {
		dprintf("module_load_symbols: missing symtab/strtab\n");
		return false;
	}

	return true;
}

const elf_sym *module_sym_by_index(const module *m, uint32_t idx) {
	if (m == NULL || idx >= m->sym_count) {
		return NULL;
	}
	return &m->symtab[idx];
}

uintptr_t module_resolve_symbol_addr(const module *m, const elf_sym *s) {
	if (m == NULL || s == NULL) {
		return 0;
	}

	if (s->st_shndx == SHN_UNDEF) {
		const char *name = m->strtab + s->st_name;
		const void *k = kernel_dlsym(name);
		return (uintptr_t) k;
	}

	if (s->st_shndx >= SHN_LORESERVE) {
		return 0;
	}

	uint8_t *base = module_section_base(m, s->st_shndx);
	if (base == NULL) {
		return 0;
	}
	return (uintptr_t) (base + s->st_value);
}

bool module_apply_relocations(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m) {
	if (file == NULL || sh == NULL || m == NULL) {
		return false;
	}
	for (size_t i = 0; i < shnum; i++) {
		if (sh[i].sh_type != SHT_RELA) {
			continue;
		}

		uint16_t tgt_index = (uint16_t) sh[i].sh_info;

		/* NEW: ignore relocations for non-ALLOC target sections (e.g. .debug*) */
		if (tgt_index >= shnum || !is_alloc_section(&sh[tgt_index])) {
			continue;
		}

		uint8_t *tgt_base = module_section_base(m, tgt_index);
		if (tgt_base == NULL) {
			dprintf("module_apply_relocations: target section base not found (idx=%u)\n", tgt_index);
			return false;
		}

		const elf_rela *rela = (const elf_rela *) (file + sh[i].sh_offset);
		size_t count = (size_t) (sh[i].sh_size / sizeof(elf_rela));

		for (size_t j = 0; j < count; j++) {
			uint32_t type = (uint32_t) ELF64_R_TYPE(rela[j].r_info);
			uint32_t si = (uint32_t) ELF64_R_SYM(rela[j].r_info);
			uintptr_t P = (uintptr_t) (tgt_base + rela[j].r_offset);
			const elf_sym *sym = module_sym_by_index(m, si);
			uintptr_t S = 0;
			int64_t A = (int64_t) rela[j].r_addend;

			if (sym != NULL) {
				S = module_resolve_symbol_addr(m, sym);
				if (S == 0 && sym->st_shndx == SHN_UNDEF) {
					const char *nm = m->strtab + sym->st_name;
					dprintf("module_apply_relocations: unresolved symbol: %s\n", nm);
					return false;
				}
			}

			switch (type) {
				case R_X86_64_64:
					*(uint64_t *) P = (uint64_t) ((uint64_t) S + (uint64_t) A);
					break;

				case R_X86_64_PC32: {
					int64_t val = (int64_t) S + A - (int64_t) P;
					if (val < INT_MIN || val > INT_MAX) {
						dprintf("module_apply_relocations: PC32 overflow\n");
						return false;
					}
					*(int32_t *) P = (int32_t) val;
					break;
				}

				/* Compilers often emit PLT32 for extern calls */
				case R_X86_64_PLT32: {
					int64_t val = (int64_t) S + A - (int64_t) P;
					if (val < INT_MIN || val > INT_MAX) {
						dprintf("module_apply_relocations: PLT32 overflow\n");
						return false;
					}
					*(int32_t *) P = (int32_t) val;
					break;
				}

				case R_X86_64_32: {
					uint64_t val = (uint64_t) S + (uint64_t) A;
					if (val > UINT_MAX) {
						dprintf("module_apply_relocations: R_X86_64_32 overflow\n");
						return false;
					}
					*(uint32_t *) P = (uint32_t) val;
					break;
				}

				case R_X86_64_32S: {
					int64_t val = (int64_t) S + A;
					if (val < INT_MIN || val > INT_MAX) {
						dprintf("module_apply_relocations: R_X86_64_32S overflow\n");
						return false;
					}
					*(int32_t *) P = (int32_t) val;
					break;
				}

				default:
					dprintf("module_apply_relocations: unsupported reloc type: %u\n", type);
					return false;
			}
		}
	}
	return true;
}

void *module_dlsym_local(const module *m, const char *name) {
	if (m == NULL || name == NULL) {
		return NULL;
	}
	for (size_t i = 0; i < m->sym_count; i++) {
		const elf_sym *s = &m->symtab[i];

		if (s->st_name == 0) {
			continue;
		}

		const char *nm = m->strtab + s->st_name;
		if (strcmp(nm, name) != 0) {
			continue;
		}

		if (s->st_shndx == SHN_UNDEF || s->st_shndx >= SHN_LORESERVE) {
			return NULL;
		}

		uint8_t *b = module_section_base(m, s->st_shndx);
		if (b == NULL) {
			return NULL;
		}
		return (void *) (b + s->st_value);
	}

	return NULL;
}

bool module_load_from_memory(const void *file, size_t len, module *out) {
	const elf_ehdr *eh;
	const elf_shdr *sh;
	size_t shnum;
	const char *shstr;

	if (file == NULL || out == NULL) {
		return false;
	}

	memset(out, 0, sizeof(*out));

	if (!parse_elf_rel_headers((const uint8_t *) file, len, &eh, &sh, &shnum, &shstr)) {
		dprintf("module_load_from_memory: header parse failed\n");
		return false;
	}

	if (!module_place_sections((const uint8_t *) file, sh, shnum, out)) {
		dprintf("module_load_from_memory: section placement failed\n");
		return false;
	}

	if (!module_load_symbols((const uint8_t *) file, sh, shnum, out)) {
		dprintf("module_load_from_memory: symbols load failed\n");
		kfree_aligned(&out->base);
		return false;
	}

	if (!module_apply_relocations((const uint8_t *) file, sh, shnum, out)) {
		dprintf("module_load_from_memory: relocation failed\n");
		kfree_aligned(&out->base);
		return false;
	}

	out->init_fn = (module_init_fn) module_dlsym_local(out, MOD_INIT_NAME_STR);
	out->exit_fn = (module_exit_fn) module_dlsym_local(out, MOD_EXIT_NAME_STR);

	if (out->init_fn == NULL) {
		dprintf("module_load_from_memory: ABI %u required, missing %s()\n", KMOD_ABI, MOD_INIT_NAME_STR);
		kfree_aligned(&out->base);
		return false;
	}

	if (!out->init_fn()) {
		if (out->exit_fn != NULL) {
			out->exit_fn();
		}
		kfree_aligned(&out->base);
		return false;
	}

	return true;
}

bool module_internal_unload(module *m) {
	if (m == NULL) {
		return false;
	}

	if (m->exit_fn != NULL) {
		if (!m->exit_fn()) {
			return false;
		}
	}

	kfree_aligned(&m->base);
	m->base = NULL;
	m->exit_fn = NULL;
	m->init_fn = NULL;
	m->placed_count = 0;
	m->symtab = NULL;
	m->strtab = NULL;
	m->sym_count = 0;
	return true;
}

static int module_compare(const void *a, const void *b, void *udata) {
	const module* ma = a;
	const module* mb = b;
	return strcmp(ma->name, mb->name);
}

static uint64_t module_hash(const void *item, uint64_t seed0, uint64_t seed1) {
	const module* a = item;
	return hashmap_sip(a->name, strlen(a->name), seed0, seed1);
}

static void module_hash_free(const void* item, void* udata) {
	const module* m = item;
	kfree(m->raw_bits);
	kfree(m->name);
}

void init_modules(void) {
	if (modules) {
		return;
	}
	modules = hashmap_new(sizeof(module), 0, mt_rand(), mt_rand(), module_hash, module_compare, module_hash_free, NULL);
}

bool load_module(const char* name) {
	module m = {};
	char path[MAX_STRINGLEN];
	snprintf(path, MAX_STRINGLEN, "/system/modules/%s.ko", name);
	m.name = strdup(name);
	if (!m.name) {
		dprintf("Out of memory\n");
		return false;
	}
	if (hashmap_get(modules, &m)) {
		dprintf("Module %s already loaded\n", name);
		kfree_null(&m.name);
		return false;
	}
	fs_directory_entry_t * f = fs_get_file_info(path);
	if (!f || (f->flags & FS_DIRECTORY) != 0) {
		kfree_null(&m.name);
		dprintf("Module %s does not exist or is a directory\n", name);
		return false;
	}
	void* module_content = kmalloc(f->size);
	if (!module_content) {
		kfree_null(&m.name);
		dprintf("Out of memory loading module %s\n", name);
		return false;
	}
	if (!fs_read_file(f, 0, f->size, module_content)) {
		kfree_null(&module_content);
		kfree_null(&m.name);
		dprintf("Failed to read file %s to load module %s: %s\n", path, name, fs_strerror(fs_get_error()));
		return false;
	}
	m.raw_bits = module_content;
	if (!module_load_from_memory(module_content, f->size, &m)) {
		kfree_null(&module_content);
		kfree_null(&m.name);
		return false;
	}
	hashmap_set(modules, &m);
	return true;
}

bool unload_module(const char* name) {
	module* mod = hashmap_get(modules, &(module){ .name = name });
	if (!mod) {
		dprintf("Module %s is not loaded\n", name);
		return false;
	}
	if (!module_internal_unload(mod)) {
		return false;
	}
	hashmap_delete(modules, mod);
	return true;
}
