/**
 * @file basic/function.c
 * @brief BASIC function/procedure functions
 */
#include <kernel.h>

#define BUILTIN_HASH_SEED0 0x9e3779b97f4a7c15ULL
#define BUILTIN_HASH_SEED1 0xc2b2ae3d27d4eb4fULL

extern bool debug;

/* Add near the top of basic/function.c, after extern bool debug */

struct builtin_int_entry {
	const char *name;
	size_t name_length;
	int64_t (*handler)(struct basic_ctx *ctx);
};

struct builtin_double_entry {
	const char *name;
	size_t name_length;
	void (*handler)(struct basic_ctx *ctx, double *res);
};

struct builtin_str_entry {
	const char *name;
	size_t name_length;
	char *(*handler)(struct basic_ctx *ctx);
};

static struct hashmap *builtin_int_map = NULL;
static struct hashmap *builtin_double_map = NULL;
static struct hashmap *builtin_str_map = NULL;

static uint64_t builtin_name_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
	const struct builtin_int_entry *entry = item;
	size_t len = entry->name_length ? entry->name_length : strlen(entry->name);

	return hashmap_sip(entry->name, len, seed0, seed1);
}

static int builtin_name_compare(const void *a, const void *b, void *udata)
{
	[[maybe_unused]] void *unused = udata;
	const struct builtin_int_entry *ea = a;
	const struct builtin_int_entry *eb = b;
	size_t la = ea->name_length ? ea->name_length : strlen(ea->name);
	size_t lb = eb->name_length ? eb->name_length : strlen(eb->name);

	if (la < lb) {
		return -1;
	}
	if (la > lb) {
		return 1;
	}
	return strcmp(ea->name, eb->name);
}

/**
 * @brief Maximum time an atomic FN evaluation may run for.
 *
 * BASIC is only pre-empted between lines. An expression containing one or more
 * FN calls must therefore be evaluated as a single atomic unit, because an FN
 * may execute arbitrary BASIC before returning a value.
 *
 * For example, in:
 *
 * A = FNfoo(1, C, 3) + FNbar(4, 5, B)
 *
 * the whole expression, including any code executed inside FNfoo() and FNbar(),
 * must complete without yielding. Splitting evaluation partway through a line
 * would require substantially more complex interpreter state management.
 *
 * This limit is a safety fuse for pathological cases such as accidental infinite
 * loops or unexpectedly expensive FN bodies. It is not a normal runtime budget,
 * and well-behaved code should complete far faster than this.
 */
#define ATOMIC_MAX_MS 250

struct basic_int_fn builtin_int[] =
{
	{ basic_abs,                 "ABS"               },
	{ basic_altkey,              "ALTKEY"            },
	{ basic_asc,                 "ASC"               },
	{ basic_capslock,            "CAPSLOCK"          },
	{ basic_cpuid,               "CPUID"             },
	{ basic_ctrlkey,             "CTRLKEY"           },
	{ basic_get_text_cur_x,      "CURRENTX"          },
	{ basic_get_text_cur_y,      "CURRENTY"          },
	{ basic_eof,                 "EOF"               },
	{ basic_existsvar_int,       "EXISTSVARI"        },
	{ basic_existsvar_real,      "EXISTSVARR"        },
	{ basic_existsvar_string,    "EXISTSVARS"        },
	{ basic_getnamecount,        "GETNAMECOUNT"      },
	{ basic_getproccount,        "GETPROCCOUNT"      },
	{ basic_getproccpuid,        "GETPROCCPUID"      },
	{ basic_getprocid,           "GETPROCID"         },
	{ basic_getprocparent,       "GETPROCPARENT"     },
	{ basic_getprocmem,          "GETPROCMEM"        },
	{ basic_getvar_int,          "GETVARI"           },
	{ basic_getsize,             "GETSIZE"           },
	{ basic_hexval,              "HEXVAL"            },
	{ basic_instr,               "INSTR"             },
	{ basic_legacy_cpuid,        "LCPUID"            },
	{ basic_len,                 "LEN"               },
	{ basic_legacy_getlastcpuid, "LGETLASTCPUID"     },
	{ basic_get_free_mem,        "MEMFREE"           },
	{ basic_get_used_mem,        "MEMUSED"           },
	{ basic_get_total_mem,       "MEMORY"            },
	{ basic_get_program_peak_mem,"MEMPEAK"           },
	{ basic_get_program_cur_mem, "MEMPROGRAM"        },
	{ basic_octval,              "OCTVAL"            },
	{ basic_openin,              "OPENIN"            },
	{ basic_openout,             "OPENOUT"           },
	{ basic_openup,              "OPENUP"            },
	{ basic_atoi,                "RADIX"             },
	{ basic_read,                "READ"              },
	{ basic_rgb,                 "RGB"               },
	{ basic_random,              "RND"               },
	{ basic_shl,                 "SHL"               },
	{ basic_shiftkey,            "SHIFTKEY"          },
	{ basic_shr,                 "SHR"               },
	{ basic_sockaccept,          "SOCKACCEPT"        },
	{ basic_sslsockaccept,       "SSLSOCKACCEPT"     },
	{ basic_socklisten,          "SOCKLISTEN"        },
	{ basic_sockstatus,          "SOCKSTATUS"        },
	{ basic_get_text_max_y,      "TERMHEIGHT"        },
	{ basic_get_text_max_x,      "TERMWIDTH"         },
	{ basic_val,                 "VAL"               },
	{ basic_sgn,                 "SGN"               },
	{ basic_int,                 "INT"               },
	{ basic_gethour,             "HOUR"              },
	{ basic_getupsecs,           "UPSECS"            },
	{ basic_getminute,           "MINUTE"            },
	{ basic_getsecond,           "SECOND"            },
	{ basic_getepoch,            "EPOCH"             },
	{ basic_getmonth,            "MONTH"             },
	{ basic_getday,              "DAY"               },
	{ basic_getweekday,          "WEEKDAY"           },
	{ basic_getyear,             "YEAR"              },
	{ basic_get_day_of_year,     "YDAY"              },
	{ basic_inportw,             "INPORTW"           },
	{ basic_inportd,             "INPORTD"           },
	{ basic_inport,              "INPORT"            },
	{ basic_udplastsourceport,   "UDPLASTSOURCEPORT" },
	{ basic_bitor,               "BITOR"             },
	{ basic_bitand,              "BITAND"            },
	{ basic_biteor,              "BITEOR"            },
	{ basic_bitnot,              "BITNOT"            },
	{ basic_bitnor,              "BITNOR"            },
	{ basic_bitnand,             "BITNAND"           },
	{ basic_bitxnor,             "BITXNOR"           },
	{ basic_bitshl,              "BITSHL"            },
	{ basic_bitshr,              "BITSHR"            },
	{ basic_bitrol,              "BITROL"            },
	{ basic_bitror,              "BITROR"            },
	{ basic_peekq,               "PEEKQ"             },
	{ basic_peekd,               "PEEKD"             },
	{ basic_peekw,               "PEEKW"             },
	{ basic_peek,                "PEEK"              },
	{ basic_memalloc,            "MEMALLOC"          },
	{ basic_filesize,            "FILESIZE"          },
	{ basic_decibels,            "DECIBELS"          },
	{ basic_spritecollide,       "SPRITECOLLIDE"     },
	{ basic_spritewidth,         "SPRITEWIDTH"       },
	{ basic_spriteheight,        "SPRITEHEIGHT"      },
	{ basic_dataread,            "DATAREAD"          },
	{ basic_ticks,               "TICKS"             },
	{ basic_min,                 "MIN"               },
	{ basic_max,                 "MAX"               },
	{ basic_spritepixel,         "SPRITEPIXEL"       },
	{ basic_spritemask,          "SPRITEMASK"        },
	{ basic_secure_random,       "SECRND"            },
	{ NULL,                      NULL                },
};

struct basic_double_fn builtin_double[] = {
	{ basic_cos,           "COS"       },
	{ basic_getvar_real,   "GETVARR"   },
	{ basic_pow,           "POW"       },
	{ basic_realval,       "REALVAL"   },
	{ basic_sin,           "SIN"       },
	{ basic_tan,           "TAN"       },
	{ basic_sqrt,          "SQRT"      },
	{ basic_sqrt,          "SQR"       },
	{ basic_atan,          "ATAN"      },
	{ basic_atan2,         "ATAN2"     },
	{ basic_ceil,          "CEIL"      },
	{ basic_round,         "ROUND"     },
	{ basic_fmod,          "FMOD"      },
	{ basic_asn,           "ASN"       },
	{ basic_acs,           "ACS"       },
	{ basic_exp,           "EXP"       },
	{ basic_log,           "LOG"       },
	{ basic_deg,           "DEG"       },
	{ basic_rad,           "RAD"       },
	{ basic_dataread_real, "DATAREADR" },
	{ basic_minr,          "MINR"      },
	{ basic_maxr,          "MAXR"      },
	{ NULL,                NULL        },
};

struct basic_str_fn builtin_str[] =
{
	{ basic_ramdisk_from_image,   "ADFSIMAGE$"    },
	{ basic_bool,                 "BOOL$"         },
	{ basic_chr,                  "CHR$"          },
	{ basic_cpugetbrand,          "CPUGETBRAND$"  },
	{ basic_cpugetvendor,         "CPUGETVENDOR$" },
	{ basic_csd,                  "CSD$"          },
	{ basic_dns,                  "DNS$"          },
	{ basic_filetype,             "FILETYPE$"     },
	{ basic_getname,              "GETNAME$"      },
	{ basic_getprocname,          "GETPROCNAME$"  },
	{ basic_getprocstate,         "GETPROCSTATE$" },
	{ basic_getvar_string,        "GETVARS$"      },
	{ basic_highlight,            "HIGHLIGHT$"    },
	{ basic_inkey,                "INKEY$"        },
	{ basic_insocket,             "INSOCKET$"     },
	{ basic_intoasc,              "INTOASC$"      },
	{ basic_left,                 "LEFT$"         },
	{ basic_ljust,                "LJUST$"        },
	{ basic_lower,                "LOWER$"        },
	{ basic_ltrim,                "LTRIM$"        },
	{ basic_mid,                  "MID$"          },
	{ basic_replace,              "REPLACE$"      },
	{ basic_netinfo,              "NETINFO$"      },
	{ basic_itoa,                 "RADIX$"        },
	{ basic_ramdisk_from_device,  "RAMDISK$"      },
	{ basic_ramdisk_from_size,    "EMPTYRAMDISK$" },
	{ basic_readstring,           "READ$"         },
	{ basic_repeat,               "REP$"          },
	{ basic_reverse,              "REVERSE$"      },
	{ basic_right,                "RIGHT$"        },
	{ basic_rjust,                "RJUST$"        },
	{ basic_rtrim,                "RTRIM$"        },
	{ basic_str,                  "STR$"          },
	{ basic_tokenize,             "TOKENIZE$"     },
	{ basic_trim,                 "TRIM$"         },
	{ basic_upper,                "UPPER$"        },
	{ basic_date,                 "DATE$"         },
	{ basic_time,                 "TIME$"         },
	{ basic_get_upstr,            "UPTIME$"       },
	{ basic_udpread,              "UDPREAD$"      },
	{ basic_udplastip,            "UDPLASTIP$"    },
	{ basic_dataread_string,      "DATAREAD$"     },
	{ basic_secure_random_string, "SECSTR$"       },
	{ NULL,                       NULL            },
};

void build_builtin_fn_maps(void)
{
	if (builtin_int_map || builtin_double_map || builtin_str_map) {
		return;
	}

	dprintf("Building function hashmaps...\n");

	builtin_int_map = hashmap_new(
		sizeof(struct builtin_int_entry),
		128,
		BUILTIN_HASH_SEED0,
		BUILTIN_HASH_SEED1,
		builtin_name_hash,
		builtin_name_compare,
		NULL,
		NULL
	);

	builtin_double_map = hashmap_new(
		sizeof(struct builtin_double_entry),
		32,
		BUILTIN_HASH_SEED0,
		BUILTIN_HASH_SEED1,
		builtin_name_hash,
		builtin_name_compare,
		NULL,
		NULL
	);

	builtin_str_map = hashmap_new(
		sizeof(struct builtin_str_entry),
		64,
		BUILTIN_HASH_SEED0,
		BUILTIN_HASH_SEED1,
		builtin_name_hash,
		builtin_name_compare,
		NULL,
		NULL
	);

	if (!builtin_int_map || !builtin_double_map || !builtin_str_map) {
		preboot_fail("Out of memory building builtin function maps");
	}

	for (int i = 0; builtin_int[i].name; ++i) {
		struct builtin_int_entry entry = {
			.name = builtin_int[i].name,
			.name_length = strlen(builtin_int[i].name),
			.handler = builtin_int[i].handler
		};

		hashmap_set(builtin_int_map, &entry);
		if (hashmap_oom(builtin_int_map)) {
			preboot_fail("Out of memory building integer builtin function map");
		}
	}

	for (int i = 0; builtin_double[i].name; ++i) {
		struct builtin_double_entry entry = {
			.name = builtin_double[i].name,
			.name_length = strlen(builtin_double[i].name),
			.handler = builtin_double[i].handler
		};

		hashmap_set(builtin_double_map, &entry);
		if (hashmap_oom(builtin_double_map)) {
			preboot_fail("Out of memory building real builtin function map");
		}
	}

	for (int i = 0; builtin_str[i].name; ++i) {
		struct builtin_str_entry entry = {
			.name = builtin_str[i].name,
			.name_length = strlen(builtin_str[i].name),
			.handler = builtin_str[i].handler
		};

		hashmap_set(builtin_str_map, &entry);
		if (hashmap_oom(builtin_str_map)) {
			preboot_fail("Out of memory building string builtin function map");
		}
	}
}

bool extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx, int* bracket_depth, char const** item_begin, struct ub_param** param) {
	if (*ctx->ptr == '(') {
		(*bracket_depth)++;
		if (*bracket_depth == 1) {
			*item_begin = ctx->ptr + 1;
		}
	} else if (*ctx->ptr == ')') {
		(*bracket_depth)--;
	}
	if ((*ctx->ptr == ',' && *bracket_depth == 1) || (*ctx->ptr == ')' && *bracket_depth == 0)) {
		// next item
		// set local vars here
		// Set ctx to item_begin, call expr(), set ctx back again. Change expr to stop on comma.
		char oldval = *ctx->ptr;
		char oldct = ctx->current_token;
		char* oldptr = (char*)ctx->ptr;
		char* oldnextptr = (char*)ctx->nextptr;
		ctx->nextptr = *item_begin;
		ctx->ptr = *item_begin;
		ctx->current_token = get_next_token(ctx);
		*oldptr = 0;
		if (*param && (*param)->name) {
			size_t len = strlen((*param)->name);
			if ((*param)->name[len - 1] == '$') {
				const char* save_ptr = ctx->ptr;
				basic_set_string_variable((*param)->name, str_expr(ctx), ctx, true, false);
				ctx->ptr = save_ptr;
			} else if ((*param)->name[len - 1] == '#') {
				double f = 0.0;
				double_expr(ctx, &f);
				basic_set_double_variable((*param)->name, f, ctx, true, false);
			} else {
				int64_t e = expr(ctx);
				basic_set_int_variable((*param)->name, e, ctx, true, false);
			}

			*param = (*param)->next;
		}
		*oldptr = oldval;
		ctx->ptr = oldptr;
		ctx->nextptr = oldnextptr;
		ctx->current_token = oldct;
		(*item_begin) = (char*)ctx->ptr + 1;
	}

	ctx->ptr++;

	if ((*bracket_depth) == 0 || *ctx->ptr == 0) {
		ctx->nextptr = ctx->ptr;
		return false;
	}
	return true;
}

const char* basic_eval_str_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	const char* rv = "";
	if (def) {
		if (!new_stack_frame(ctx)) {
			return "";
		}
		init_local_heap(ctx);

		int bracket_depth = 0;
		const char* item_begin = ctx->ptr;
		struct ub_param* param = def->params;
		while (extract_comma_list(def, ctx, &bracket_depth, &item_begin, &param));

		struct basic_ctx* atomic = basic_clone(ctx);
		if (!atomic) {
			return "";
		}
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		process_t proc;
		memcpy(&proc, proc_cur(logical_cpu_id()), sizeof(process_t));
		proc.code = atomic;
		atomic->proc = &proc;
		uint64_t start = get_ticks();
		while (!basic_finished(atomic)) {
			if (proc.check_idle && proc.check_idle(&proc, proc.idle_context)) {
				__builtin_ia32_pause();
				continue;
			}
			proc_set_idle(&proc, NULL, NULL);
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				ctx->ended = atomic->ended;
				break;
			}
			if (get_ticks() - start > ATOMIC_MAX_MS) {
				tokenizer_error_printf(ctx, "FN%s: atomic function timed out", fn_name);
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = gc_strdup(ctx, (const char*)atomic->fn_return);
			if (!rv) {
				return "";
			}
		}

		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;
		ctx->sounds           = atomic->sounds;
		ctx->data_offset      = atomic->data_offset;
		ctx->graphics_colour  = atomic->graphics_colour;

		memcpy(ctx->audio_streams, atomic->audio_streams, sizeof(ctx->audio_streams));
		memcpy(ctx->envelopes, atomic->envelopes, sizeof(ctx->envelopes));

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		pop_stack_frame(ctx);

		return rv;
	}
	tokenizer_error_print(ctx, "No such string FN");
	return rv;
}

/**
 * @brief Check if a function name is a builtin function returning an integer value,
 * if it is, call its handler and set its return value.
 * 
 * @param fn_name function name to look for
 * @param ctx interpreter context
 * @param res pointer to return value of function
 * @return uint64_t true/false return
 */
char basic_builtin_int_fn(const char* fn_name, struct basic_ctx* ctx, int64_t* res)
{
	struct builtin_int_entry key = {
		.name = fn_name,
	};
	struct builtin_int_entry *entry = hashmap_get(builtin_int_map, &key);

	if (!entry) {
		return 0;
	}

	*res = entry->handler(ctx);
	return 1;
}

char basic_builtin_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res)
{
	struct builtin_double_entry key = {
		.name = fn_name,
	};
	struct builtin_double_entry *entry = hashmap_get(builtin_double_map, &key);

	if (!entry) {
		return 0;
	}

	entry->handler(ctx, res);
	return 1;
}

/**
 * @brief Check if a function name is a builtin function returning a string,
 * if it is, call its handler and set its return value.
 *
 * @param fn_name function name to look for
 * @param ctx interpreter context
 * @param res pointer to return value of function
 * @return uint64_t true/false return
 */
char basic_builtin_str_fn(const char* fn_name, struct basic_ctx* ctx, char** res)
{
	struct builtin_str_entry key = {
		.name = fn_name,
	};
	struct builtin_str_entry *entry = hashmap_get(builtin_str_map, &key);

	if (!entry) {
		return 0;
	}

	*res = entry->handler(ctx);
	return *res ? 1 : 0;
}

int64_t basic_eval_int_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	int64_t rv = 0;
	if (def) {
		if (!new_stack_frame(ctx)) {
			return 0;
		}
		init_local_heap(ctx);

		int bracket_depth = 0;
		const char* item_begin = ctx->ptr;
		struct ub_param* param = def->params;
		while (extract_comma_list(def, ctx, &bracket_depth, &item_begin, &param));

		struct basic_ctx* atomic = basic_clone(ctx);
		if (!atomic) {
			return 0;
		}
		atomic->fn_type = RT_INT;
		jump_linenum(def->line, atomic);

		process_t proc;
		memcpy(&proc, proc_cur(logical_cpu_id()), sizeof(process_t));
		proc.code = atomic;
		atomic->proc = &proc;
		uint64_t start = get_ticks();
		while (!basic_finished(atomic)) {
			if (proc.check_idle && proc.check_idle(&proc, proc.idle_context)) {
				__builtin_ia32_pause();
				continue;
			}
			proc_set_idle(&proc, NULL, NULL);
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				ctx->ended = atomic->ended;
				dprintf("Function errored, atomic->ended=%d\n", atomic->ended);
				break;
			}
			if (get_ticks() - start > ATOMIC_MAX_MS) {
				tokenizer_error_printf(ctx, "FN%s: atomic function timed out", fn_name);
				break;
			}
		}
		rv = (int64_t)atomic->fn_return;

		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;
		ctx->sounds           = atomic->sounds;
		ctx->data_offset      = atomic->data_offset;
		ctx->graphics_colour  = atomic->graphics_colour;

		memcpy(ctx->audio_streams, atomic->audio_streams, sizeof(ctx->audio_streams));
		memcpy(ctx->envelopes, atomic->envelopes, sizeof(ctx->envelopes));

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		pop_stack_frame(ctx);

		return rv;
	}
	tokenizer_error_print(ctx, "No such integer FN");
	return 0;
}

void basic_eval_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	if (def) {
		if (!new_stack_frame(ctx)) {
			*res = 0;
			return;
		}
		init_local_heap(ctx);

		int bracket_depth = 0;
		const char* item_begin = ctx->ptr;
		struct ub_param* param = def->params;
		while (extract_comma_list(def, ctx, &bracket_depth, &item_begin, &param));

		struct basic_ctx* atomic = basic_clone(ctx);
		if (!atomic) {
			*res = 0;
			return;
		}
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		process_t proc;
		memcpy(&proc, proc_cur(logical_cpu_id()), sizeof(process_t));
		proc.code = atomic;
		atomic->proc = &proc;
		uint64_t start = get_ticks();
		while (!basic_finished(atomic)) {
			if (proc.check_idle && proc.check_idle(&proc, proc.idle_context)) {
				__builtin_ia32_pause();
				continue;
			}
			proc_set_idle(&proc, NULL, NULL);
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				ctx->ended = atomic->ended;
				break;
			}
			if (get_ticks() - start > ATOMIC_MAX_MS) {
				tokenizer_error_printf(ctx, "FN%s: atomic function timed out", fn_name);
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			*res = *((double*)atomic->fn_return);
		}

		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;
		ctx->sounds           = atomic->sounds;
		ctx->data_offset      = atomic->data_offset;
		ctx->graphics_colour  = atomic->graphics_colour;

		memcpy(ctx->audio_streams, atomic->audio_streams, sizeof(ctx->audio_streams));
		memcpy(ctx->envelopes, atomic->envelopes, sizeof(ctx->envelopes));

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		pop_stack_frame(ctx);

		return;
	}
	tokenizer_error_print(ctx, "No such real FN");
	*res = 0.0;
	return;
}

bool basic_parse_fn(struct basic_ctx* ctx)
{
	int currentline = 0;
	char* program = ctx->ptr;

	while (true) {
		currentline = atoi(program);
		char const* linestart = program;
		while (*program != '\n' && *program != 0) {
			++program;
		}

		char const* lineend = program;

		char* linetext = buddy_malloc(ctx->allocator, lineend - linestart + 1);
		if (!linetext) {
			return false;
		}
		strlcpy(linetext, linestart, lineend - linestart + 1);

		char* search = linetext;

		while (*search++ >= '0' && *search <= '9') {
			search++;
		}
		--search;

		while (*search++ == ' ');
		--search;

		if (!strncmp(search, "DEF ", 4)) {
			search += 4;
			ub_fn_type type = FT_FN;
			if (!strncmp(search, "FN", 2)) {
				search += 2;
				while (*search++ == ' ');
				type = FT_FN;
			} else if (!strncmp(search, "PROC", 4)) {
				search += 4;
				while (*search++ == ' ');
				type = FT_PROC;
			}

			char name[MAX_STRINGLEN];
			int ni = 0;
			--search;
			while (ni < MAX_STRINGLEN - 2 && *search != '\n' && *search != 0 && *search != '(') {
				name[ni++] = *search++;
			}
			name[ni] = 0;
			struct ub_proc_fn_def* exist_def = basic_find_fn(name, ctx);
			if (exist_def) {
				tokenizer_error_printf(ctx, "Duplicate function name '%s'", name);
				buddy_free(ctx->allocator, linetext);
				return false;
			}

			struct ub_proc_fn_def def = { 0 };
			def.name = buddy_strdup(ctx->allocator, name);
			if (!def.name) {
				tokenizer_error_printf(ctx, "Out of memory parsing functions");
				return false;
			}
			def.name_length = strlen(name);
			def.type = type;
			def.line = currentline;
			def.params = NULL;

			if (*search == '(') {
				search++;
				// Parse parameters
				char pname[MAX_STRINGLEN];
				int pni = 0;
				while (*search != 0 && *search != '\n') {
					if (pni < MAX_STRINGLEN - 1 && *search != ',' && *search != ')' && *search != ' ') {
						pname[pni++] = *search;
					}

					if (*search == ',' || *search == ')') {
						pname[pni] = 0;
						struct ub_param* par = buddy_malloc(ctx->allocator, sizeof(struct ub_param));
						if (!par) {
							tokenizer_error_printf(ctx, "Out of memory parsing function parameters");
							return false;
						}
						par->next = NULL;
						par->name = buddy_strdup(ctx->allocator, pname);
						if (!par->name) {
							tokenizer_error_printf(ctx, "Out of memory parsing function parameters");
							return false;
						}

						if (def.params == NULL) {
							def.params = par;
						} else {
							struct ub_param* cur = def.params;
							for (; cur; cur = cur->next) {
								if (cur->next == NULL) {
									cur->next = par;
									break;
								}
							}
						}
						if (*search == ')') {
							break;
						}
						pni = 0;
					}
					search++;
				}
			}

			if (!hashmap_set(ctx->defs, &def) && hashmap_oom(ctx->defs)) {
				tokenizer_error_printf(ctx, "Out of memory parsing functions");
				for (; def.params; ) {
					void* next = def.params->next;
					buddy_free(ctx->allocator, def.params->name);
					buddy_free(ctx->allocator, def.params);
					def.params = next;
				}
				buddy_free(ctx->allocator, (void*)def.name);
				buddy_free(ctx->allocator, linetext);
				return false;
			}
		}

		while (*program == '\n') {
			++program;
		}
		buddy_free(ctx->allocator, linetext);
		if (!*program) {
			break;
		}
	}

	ctx->ended = false;
	return true;
}

struct ub_proc_fn_def* basic_find_fn(const char* name, struct basic_ctx* ctx)
{
	if (!name) {
		return NULL;
	}
	return hashmap_get(ctx->defs, &(ub_proc_fn_def) { .name = name });
}

void basic_free_defs(struct basic_ctx* ctx)
{
	if (!ctx->defs) {
		return;
	}

	size_t i = 0;
	void* item = NULL;
	while (hashmap_iter(ctx->defs, &i, &item)) {
		struct ub_proc_fn_def* def = item;
		for (; def->params; ) {
			void* next = def->params->next;
			buddy_free(ctx->allocator, def->params->name);
			buddy_free(ctx->allocator, def->params);
			def->params = next;
		}
		buddy_free(ctx->allocator, def->name);
	}

	hashmap_free(ctx->defs);
	ctx->defs = NULL;
}

bool is_builtin_double_fn(const char* fn_name)
{
	struct builtin_double_entry key = {
		.name = fn_name,
		.name_length = strlen(fn_name),
		.handler = NULL
	};

	return hashmap_get(builtin_double_map, &key) != NULL;
}

void proc_statement(struct basic_ctx* ctx)
{
	char procname[MAX_STRINGLEN];
	char* p = procname;
	size_t procnamelen = 0;
	accept_or_return(PROC, ctx);
	while (*ctx->ptr != '\n' && *ctx->ptr != 0  && *ctx->ptr != '(' && procnamelen < MAX_STRINGLEN - 1) {
		if (*ctx->ptr != ' ') {
			*(p++) = *(ctx->ptr++);
		}
		procnamelen++;
	}
	*p++ = 0;
	struct ub_proc_fn_def* def = basic_find_fn(procname, ctx);
	if (def) {
		if (*ctx->ptr == '(' && *(ctx->ptr + 1) != ')') {
			if (!new_stack_frame(ctx)) {
				return;
			}
			init_local_heap(ctx);

			int bracket_depth = 0;
			const char* item_begin = ctx->ptr;
			struct ub_param* param = def->params;
			while (extract_comma_list(def, ctx, &bracket_depth, &item_begin, &param));

			pop_stack_frame(ctx);
		} else {
			if (!new_stack_frame(ctx)) {
				return;
			}
			init_local_heap(ctx);
			pop_stack_frame(ctx);
		}

		ctx->fn_type_stack[ctx->call_stack_ptr] = ctx->fn_type; // save caller’s type
		ctx->fn_type = RT_NONE;
		ctx->loop_state_stack[ctx->call_stack_ptr].for_stack_ptr = ctx->for_stack_ptr;
		ctx->loop_state_stack[ctx->call_stack_ptr].while_stack_ptr = ctx->while_stack_ptr;
		ctx->loop_state_stack[ctx->call_stack_ptr].repeat_stack_ptr = ctx->repeat_stack_ptr;

		while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
			tokenizer_next(ctx);
		}
		accept_or_return(NEWLINE, ctx);

		if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
			ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
			basic_debug("PROC from %lu returning line %lu, PROC %s on line %lu\n", ctx->current_linenum, ctx->call_stack[ctx->call_stack_ptr], procname, def->line);
			if (!new_stack_frame(ctx)) {
				return;
			}
			jump_linenum(def->line, ctx);
		} else {
			tokenizer_error_print(ctx, "PROC: stack exhausted");
		}
		return;
	}
	tokenizer_error_printf(ctx, "No such PROC %s", procname);
}

void def_statement(struct basic_ctx* ctx)
{
	// Because the function or procedure definition is pre-parsed by basic_init(),
	// we just skip the entire line moving to the next if we hit a DEF statement.
	// in the future we should check if the interpreter is actually calling a FN,
	// to check we don't fall through into a function.
	accept_or_return(DEF, ctx);
	while (*ctx->nextptr && *ctx->nextptr != '\n') {
		ctx->nextptr++;
	}
	tokenizer_next(ctx);
	accept_or_return(NEWLINE, ctx);
}

void eq_statement(struct basic_ctx* ctx)
{
	basic_debug("eq_statement\n");
	accept_or_return(EQUALS, ctx);

	if (ctx->fn_type == RT_STRING) {
		basic_debug("eq_statement return string\n");
		ctx->fn_return = (void*)str_expr(ctx);
	} else if (ctx->fn_type == RT_FLOAT) {
		basic_debug("eq_statement return double\n");
		double_expr(ctx, (void*)&ctx->fn_return);
	} else if (ctx->fn_type == RT_INT) {
		ctx->fn_return = (void*)expr(ctx);
		basic_debug("eq_statement return int %ld\n", (int64_t)ctx->fn_return);
	} else if (ctx->fn_type == RT_NONE) {
		tokenizer_error_print(ctx, "Can't return a value from a PROC");
		return;
	} else {
		dprintf("EQ statement: fn type??? %d\n", ctx->fn_type);
	}

	//accept_or_return(NEWLINE, ctx);

	ctx->ended = true;
}

void endproc_statement(struct basic_ctx* ctx)
{
	accept_or_return(ENDPROC, ctx);
	accept_or_return(NEWLINE, ctx);

	/* Validate the *current* frame: ENDPROC is only legal in a PROC body. */
	if (ctx->fn_type != RT_NONE && ctx->fn_type != RT_MAIN) {
		tokenizer_error_print(ctx, "Can't ENDPROC from a FN");
		return;
	}

	if (ctx->call_stack_ptr > 0) {
		free_local_heap(ctx);
		pop_stack_frame(ctx);

		/* Now restore the *caller*'s return type. */
		ctx->fn_type = ctx->fn_type_stack[ctx->call_stack_ptr];
		while (ctx->for_stack_ptr > ctx->loop_state_stack[ctx->call_stack_ptr].for_stack_ptr) {
			ctx->for_stack_ptr--;
			buddy_free(ctx->allocator, ctx->for_stack[ctx->for_stack_ptr].for_variable);
		}
		ctx->while_stack_ptr = ctx->loop_state_stack[ctx->call_stack_ptr].while_stack_ptr;
		ctx->repeat_stack_ptr = ctx->loop_state_stack[ctx->call_stack_ptr].repeat_stack_ptr;

		ctx->if_nest_level = 0; // If we exit a proc, we clear the nest level

		basic_debug("ENDPROC back to line %lu stack pos %lu\n", ctx->call_stack[ctx->call_stack_ptr], ctx->call_stack_ptr);
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "ENDPROC when not inside PROC");
	}
}
