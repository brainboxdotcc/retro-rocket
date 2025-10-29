/**
 * @file module.h
 * @brief Minimal ELF64 definitions and the run-time module loader interface
 *
 * Provides:
 *  - A compact, libc-free subset of ELF64 constants and structs used by the loader
 *  - The @ref module descriptor describing a loaded, relocated module image
 *  - The public API for loading, relocating, resolving, and unloading modules
 *
 * @note These definitions intentionally avoid pulling in <elf.h> to keep the
 *       kernel freestanding. Layout matches the System V ABI for ELF64
 */
#pragma once

#include <kernel.h>
#include <stdbool.h>

/**
 * @brief module ABI version the kernel expects
 *
 * @note If we change the ABI at all, we must bump this by 1.
 * It is used in the names of the init/exit functions of each module,
 * so an incompatible module is outright rejected before it can wreak
 * havoc in the kernel space.
 */
#define KMOD_ABI 100

/* ELF identification */
#define EI_NIDENT      16   /**< Size of e_ident[] array */

#define EI_MAG0        0    /**< e_ident index: magic[0] = 0x7F */
#define EI_MAG1        1    /**< e_ident index: magic[1] = 'E' */
#define EI_MAG2        2    /**< e_ident index: magic[2] = 'L' */
#define EI_MAG3        3    /**< e_ident index: magic[3] = 'F' */
#define EI_CLASS       4    /**< e_ident index: file class (ELFCLASS*) */

#define ELFMAG0        0x7f /**< Magic byte 0 */
#define ELFMAG1        'E'  /**< Magic byte 1 */
#define ELFMAG2        'L'  /**< Magic byte 2 */
#define ELFMAG3        'F'  /**< Magic byte 3 */

#define ELFCLASS64     2    /**< 64-bit objects */

/* File type & machine */
#define ET_REL         1    /**< Relocatable object file */
#define EM_X86_64      62   /**< AMD64 / x86-64 architecture */

#define ELF64_ST_TYPE(i)   ((uint8_t)((i) & 0x0F))
#define STT_NOTYPE   0
#define STT_FUNC     2

/* Section types (sh_type) */
#define SHT_NULL       0    /**< Inactive section header */
#define SHT_PROGBITS   1    /**< Program-defined contents */
#define SHT_SYMTAB     2    /**< Static symbol table */
#define SHT_STRTAB     3    /**< String table */
#define SHT_RELA       4    /**< Relocations with addends */
#define SHT_NOBITS     8    /**< Occupies no space in file (e.g., .bss) */

/* Section flags (sh_flags) */
#define SHF_WRITE      0x1      /**< Writable at run-time */
#define SHF_ALLOC      0x2      /**< Occupies memory after loading */
#define SHF_EXECINSTR  0x4      /**< Contains executable instructions */

/* Special section indices */
#define SHN_UNDEF      0        /**< Undefined / external reference */
#define SHN_LORESERVE  0xff00u  /**< Lower bound of reserved range */

/* Symbol binding */
#define STB_LOCAL      0    /**< Local (not visible outside object) */
#define STB_GLOBAL     1    /**< Global symbol */
#define STB_WEAK       2    /**< Weak symbol */
#define ELF64_ST_BIND(i)   ((unsigned)((i) >> 4)) /**< Extract STB_* binding from st_info */

/* Relocation helpers */
#define ELF64_R_SYM(i)     ((uint32_t)((i) >> 32))         /**< Extract symbol index from r_info */
#define ELF64_R_TYPE(i)    ((uint32_t)((i) & 0xffffffffu)) /**< Extract relocation type from r_info */

/* x86_64 relocation types */
#define R_X86_64_64    1   /**< 64-bit absolute address */
#define R_X86_64_PC32  2   /**< 32-bit signed PC-relative */
#define R_X86_64_PLT32 4   /**< 32-bit PLT 32 */
#define R_X86_64_32    10  /**< 32-bit zero-extended absolute */
#define R_X86_64_32S   11  /**< 32-bit sign-extended absolute */

/**
 * @brief ELF file header (ELF64)
 */
typedef struct elf_ehdr {
	unsigned char e_ident[EI_NIDENT]; /**< ELF identification bytes */
	uint16_t e_type;             /**< Object file type (e.g. ET_REL) */
	uint16_t e_machine;          /**< Target architecture (e.g. EM_X86_64) */
	uint32_t e_version;          /**< Object file version */
	uint64_t e_entry;            /**< Entry point (unused for ET_REL) */
	uint64_t e_phoff;            /**< Program header table file offset */
	uint64_t e_shoff;            /**< Section header table file offset */
	uint32_t e_flags;            /**< Processor-specific flags */
	uint16_t e_ehsize;           /**< ELF header size in bytes */
	uint16_t e_phentsize;        /**< Size of one program header entry */
	uint16_t e_phnum;            /**< Number of program header entries */
	uint16_t e_shentsize;        /**< Size of one section header entry */
	uint16_t e_shnum;            /**< Number of section header entries */
	uint16_t e_shstrndx;         /**< Section name string table index */
} elf_ehdr;

/**
 * @brief ELF section header (ELF64)
 */
typedef struct elf_shdr {
	uint32_t sh_name;      /**< Offset into section-name string table */
	uint32_t sh_type;      /**< Section type (SHT_*) */
	uint64_t sh_flags;     /**< Section flags (SHF_*) */
	uint64_t sh_addr;      /**< Virtual address (unused for ET_REL) */
	uint64_t sh_offset;    /**< File offset of section contents */
	uint64_t sh_size;      /**< Section size in bytes */
	uint32_t sh_link;      /**< Section link (type-dependent) */
	uint32_t sh_info;      /**< Extra info (type-dependent) */
	uint64_t sh_addralign; /**< Required alignment of section in memory */
	uint64_t sh_entsize;   /**< Entry size for fixed-size tables */
} elf_shdr;

/**
 * @brief ELF symbol table entry (ELF64)
 */
typedef struct elf_sym {
	uint32_t st_name;  /**< Offset into the string table */
	unsigned char st_info;  /**< Binding and type (use ELF64_ST_BIND to extract) */
	unsigned char st_other; /**< Visibility (not used here) */
	uint16_t st_shndx; /**< Section index (SHN_* or real section) */
	uint64_t st_value; /**< Value or offset; for ET_REL, offset within section */
	uint64_t st_size;  /**< Symbol size in bytes */
} elf_sym;

/**
 * @brief ELF relocation with addend (ELF64)
 */
typedef struct elf_rela {
	uint64_t r_offset; /**< Offset within the target section */
	uint64_t r_info;   /**< Packed type and symbol index */
	int64_t r_addend; /**< Signed addend */
} elf_rela;

typedef bool (*module_init_fn)(void);  /**< Prototype for module initialiser */
typedef bool (*module_exit_fn)(void);  /**< Prototype for module finaliser */

/**
 * @brief State for FSM for loadorder.conf
 */
enum loadorder_parse_state_t {
	SEARCH_ALIAS, /**< Searching for alias name in square brackets */
	READ_ALIAS, /**< Reading alias name in square brackets */
	READ_VENDOR, /**< Reading vendor ID */
	READ_DEVICE, /**< Reading device ID */
	READ_TYPE, /**< Reading class type */
	READ_MODLIST, /**< Reading module list */
};

/**
 * @brief Set this prefix on all module_init/module_exit functions in modules
 */
#define EXPORTED __attribute__((visibility("default")))

/**
 * @brief helpers to form symbol names
 */
#define MOD_CAT2(a, b) a##b
#define MOD_CAT(a, b)  MOD_CAT2(a,b)
#define MOD_INIT_SYM(ver) MOD_CAT(mod_init_v, ver)
#define MOD_EXIT_SYM(ver) MOD_CAT(mod_exit_v, ver)

/* stringification */
#define STR2(x) #x
#define STR(x) STR2(x)

/**
 * @brief canonical names as strings
 */
#define MOD_INIT_NAME_STR "mod_init_v" STR(KMOD_ABI)
#define MOD_EXIT_NAME_STR "mod_exit_v" STR(KMOD_ABI)

/**
 * @brief Describes a loaded and relocated module image
 *
 * The loader places all SHF_ALLOC sections into a single contiguous allocation,
 * preserves the module's static symbol/string tables for diagnostics and local
 * lookup, and resolves conventional entry points named mod_init and mod_exit
 */
typedef struct module {
	const char *name;                /**< Module name */
	uint8_t *base;                        /**< Base of contiguous allocation holding SHF_ALLOC sections */
	void *raw_bits;                        /**< For symbol resolution by other modules */
	size_t size;                        /**< Total size of the contiguous allocation */
	const elf_sym *symtab;                /**< Pointer to the module's SHT_SYMTAB within the file buffer */
	size_t sym_count;                /**< Number of entries in symtab */
	const char *strtab;                /**< Pointer to the module's SHT_STRTAB within the file buffer */
	struct {
		uint16_t shndx;                /**< Section index in the module's section table */
		uint8_t *addr;                /**< Run-time address where this section was placed */
	} placed[64];                        /**< Mapping of placed section indices to run-time addresses */
	size_t placed_count;                /**< Number of valid entries in placed */
	module_init_fn init_fn;                /**< Module initialiser (called on load) */
	module_exit_fn exit_fn;                /**< Module finaliser (called on unload) */
} module;

/**
 * @brief Load and relocate a module from an in-memory ELF64 ET_REL buffer, then call its initialiser
 *
 * Validates the ELF header, allocates a single contiguous region for all SHF_ALLOC sections,
 * copies/zeros contents with correct alignment, applies all SHT_RELA relocations, resolves
 * undefined globals against the kernel dynamic symbol table (from kernel.sym),
 * resolves versioned mod_init/mod_exit, then calls mod_init
 *
 * @param file pointer to the ELF bytes
 * @param len  size of the ELF buffer in bytes
 * @param out  output descriptor populated on success; zeroed on entry
 * @return true on success, false on error
 *
 * @note the input buffer is not retained and may be freed after return
 * @warning fails if any relocation overflows or an undefined symbol cannot be resolved
 */
bool module_load_from_memory(const void *file, size_t len, module *out);

/**
 * @brief Unload a module by calling its finaliser and freeing its memory
 *
 * Calls mod_exit if present, frees the contiguous allocation, and clears the descriptor
 *
 * @param m module descriptor previously returned by module_load_from_memory
 * @return true on success, false if @p m is NULL or teardown fails
 */
bool module_internal_unload(module *m);

/**
 * @brief Resolve a kernel global by name via the kernel dynamic symbol table
 *
 * Requires the kernel to be linked with --export-dynamic and to KEEP .dynsym and .dynstr
 *
 * @param name NUL-terminated symbol name
 * @return symbol address on success, NULL if not found
 */
const void *kernel_dlsym(const char *name);

/**
 * @brief Resolve a symbol by name within a loaded module after relocation
 *
 * Looks up the symbol in the module’s own SHT_SYMTAB/STRTAB and computes its run-time address
 *
 * @param m    loaded module descriptor
 * @param name NUL-terminated symbol name
 * @return address within the module image, or NULL if not found or undefined
 */
void *module_dlsym_local(const module *m, const char *name);

/**
 * @brief Parse and validate the ELF64 ET_REL header and section table
 *
 * Performs basic structural checks and returns pointers into the provided buffer
 *
 * @param file  pointer to the ELF bytes
 * @param len   size of the buffer in bytes
 * @param eh    out: pointer to the ELF header inside @p file
 * @param sh    out: pointer to the section header table inside @p file
 * @param shnum out: number of section headers
 * @param shstr out: pointer to the section-name string table inside @p file
 * @return true if the buffer looks like a valid ELF64 ET_REL object, else false
 */
bool parse_elf_rel_headers(const uint8_t *file, size_t len, const elf_ehdr **eh, const elf_shdr **sh, size_t *shnum, const char **shstr);

/**
 * @brief Predicate for sections that must be placed in memory at load time
 *
 * @param h section header
 * @return true if SHF_ALLOC is set, else false
 */
bool is_alloc_section(const elf_shdr *h);

/**
 * @brief Allocate one contiguous block and place all SHF_ALLOC sections into it
 *
 * Computes total size with per-section alignment, allocates with kmalloc_aligned,
 * copies SHT_PROGBITS contents, zeros SHT_NOBITS, and records the run-time base of each placed section
 *
 * @param file  pointer to the ELF bytes
 * @param sh    section header table
 * @param shnum number of section headers
 * @param m     module descriptor to populate (base, size, placed[], placed_count)
 * @return true on success, false on error
 */
bool module_place_sections(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m);

/**
 * @brief Return the run-time base address for a placed section
 *
 * @param m     module descriptor
 * @param shndx section index
 * @return base address where that section was placed, or NULL if unknown
 */
uint8_t *module_section_base(const module *m, uint16_t shndx);

/**
 * @brief Locate the module’s static symbol table and its associated string table
 *
 * Populates m->symtab, m->sym_count and m->strtab
 *
 * @param file  pointer to the ELF bytes
 * @param sh    section header table
 * @param shnum number of section headers
 * @param m     module descriptor to populate
 * @return true on success, false if tables are missing
 */
bool module_load_symbols(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m);

/**
 * @brief Index into the module symbol table
 *
 * @param m   module descriptor
 * @param idx symbol index (0..sym_count-1)
 * @return pointer to the symbol, or NULL if out of range
 */
const elf_sym *module_sym_by_index(const module *m, uint32_t idx);

/**
 * @brief Compute a symbol’s absolute run-time address post-relocation
 *
 * For SHN_UNDEF symbols resolves against the kernel via kernel_dlsym
 * For defined symbols returns placed_section_base + st_value
 *
 * @param m module descriptor
 * @param s symbol entry
 * @return absolute address on success, 0 on failure
 */
uintptr_t module_resolve_symbol_addr(const module *m, const elf_sym *s);

/**
 * @brief Apply all SHT_RELA relocations for the module
 *
 * Supports R_X86_64_64, R_X86_64_PC32, R_X86_64_32 and R_X86_64_32S
 * Undefined globals are resolved via kernel_dlsym; overflows cause failure
 *
 * @param file  pointer to the ELF bytes
 * @param sh    section header table
 * @param shnum number of section headers
 * @param m     module descriptor
 * @return true on success, false on error
 *
 * @warning R_X86_64_PC32 must only target intra-module references
 *          externals should appear as R_X86_64_64 when modules are built with -mcmodel=large -fno-pic -fno-plt
 */
bool module_apply_relocations(const uint8_t *file, const elf_shdr *sh, size_t shnum, module *m);

/**
 * @brief Initialise the module registry and loader state
 *
 * Creates the in-kernel hashmap used to track loaded modules keyed by name
 * Must be called once before any other module operations
 */
void init_modules(void);

/**
 * @brief Load and start a module by name
 *
 * Constructs the path /system/modules/<name>.ko, reads the file, relocates it,
 * resolves externals against the kernel symbol index, and calls mod_init
 * Returns false if the module is already loaded or on any failure
 *
 * @param name NUL-terminated module base name without extension or path
 * @return true on success, false on error
 */
bool load_module(const char *name);

/**
 * @brief Stop and unload a loaded module by name
 *
 * Locates the module in the registry, calls mod_exit if present, frees all
 * allocations associated with the image, and removes the entry from the registry
 *
 * @param name NUL-terminated module base name
 * @return true on success, false if not found or unload failed
 */
bool unload_module(const char *name);

/**
 * @brief Resolves a code address to module, nearest symbol, and byte offset.
 *
 * Scans the registered module list for a text segment containing @p addr.
 * If found, selects the symbol whose start address is the greatest value
 * less than or equal to @p addr and reports the module name, symbol name,
 * and offset from that symbol. If the address lies within a module but no
 * symbol begins at or before @p addr, the module name is still returned and
 * the symbol name is set to NULL with a zero offset.
 *
 * @param addr         Absolute code address to resolve.
 * @param modname_out  [out, optional] Receives a pointer to the owning module
 *                     name (internal storage; do not free). Ignored if NULL.
 * @param symname_out  [out, optional] Receives a pointer to the resolved
 *                     symbol name (internal storage; do not free), or NULL if
 *                     no symbol matched within the module. Ignored if NULL.
 * @param offset_out   [out, optional] Receives the byte offset
 *                     (addr - symbol_start) when a symbol is found, or 0 if
 *                     no symbol matched. Ignored if NULL.
 *
 * @return true if @p addr fell within a known module’s text range (whether
 *         or not a symbol was found); false if no containing module existed.
 *
 * @note Returned name pointers remain valid for as long as the underlying
 *       module remains loaded.
 */
bool module_addr_to_symbol(uintptr_t addr, const char **modname_out, const char **symname_out, uint64_t *offset_out);

/**
 * @brief Parses a load-order alias and loads the first matching module set.
 *
 * Opens and parses `/system/config/loadorder.conf`, searching for a section
 * header matching the given @p alias (e.g., `[net]`). Within that section,
 * each non-comment line is expected to contain four tab/space-separated
 * fields:
 *
 *     VENDOR  DEVICE  TYPE  MOD1[, MOD2, ...]
 *
 * where VENDOR and DEVICE are hexadecimal PCI IDs (or `*` as a wildcard),
 * TYPE is a hexadecimal PCI class/subclass identifier (or `*` as a wildcard),
 * and the module list is a comma-separated list of module names. The first
 * line whose PCI criteria match the current machine (or a full wildcard line)
 * is selected; each listed module is then loaded in order.
 *
 * @param alias  Alias section name to resolve (without brackets).
 *
 * @return true if a matching alias section was found and all modules on the
 *         first matching line were successfully loaded; false otherwise
 *         (including parse errors, no matching PCI line, I/O failure, or any
 *         module load failure).
 *
 * @remarks
 *  - Lines beginning with `#` are treated as comments.
 *  - Whitespace around module names is trimmed; empty tokens are ignored.
 *  - Only the first matching line within the alias section is acted upon.
 *  - Wildcards: `*` applies unconditionally.
 */
bool module_parse_alias(const char *alias);