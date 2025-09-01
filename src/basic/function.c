/**
 * @file basic/function.c
 * @brief BASIC function/procedure functions
 */
#include <kernel.h>

extern bool debug;

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
	{ NULL,                      NULL                },
};

struct basic_double_fn builtin_double[] = {
	{ basic_cos,         "COS"     },
	{ basic_getvar_real, "GETVARR" },
	{ basic_pow,         "POW"     },
	{ basic_realval,     "REALVAL" },
	{ basic_sin,         "SIN"     },
	{ basic_tan,         "TAN"     },
	{ basic_sqrt,        "SQRT"    },
	{ basic_sqrt,        "SQR"     },
	{ basic_atan,        "ATAN"    },
	{ basic_atan2,       "ATAN2"   },
	{ basic_ceil,        "CEIL"    },
	{ basic_round,       "ROUND"   },
	{ basic_fmod,        "FMOD"    },
	{ basic_asn,         "ASN"     },
	{ basic_acs,         "ACS"     },
	{ basic_exp,         "EXP"     },
	{ basic_log,         "LOG"     },
	{ basic_deg,         "DEG"     },
	{ basic_rad,         "RAD"     },
	{ NULL,              NULL      },
};

struct basic_str_fn builtin_str[] =
{
	{ basic_bool,                "BOOL$"         },
	{ basic_chr,                 "CHR$"          },
	{ basic_cpugetbrand,         "CPUGETBRAND$"  },
	{ basic_cpugetvendor,        "CPUGETVENDOR$" },
	{ basic_csd,                 "CSD$"          },
	{ basic_dns,                 "DNS$"          },
	{ basic_filetype,            "FILETYPE$"     },
	{ basic_getname,             "GETNAME$"      },
	{ basic_getprocname,         "GETPROCNAME$"  },
	{ basic_getprocstate,        "GETPROCSTATE$" },
	{ basic_getvar_string,       "GETVARS$"      },
	{ basic_inkey,               "INKEY$"        },
	{ basic_insocket,            "INSOCKET$"     },
	{ basic_intoasc,             "INTOASC$"      },
	{ basic_left,                "LEFT$"         },
	{ basic_ljust,               "LJUST$"        },
	{ basic_lower,               "LOWER$"        },
	{ basic_ltrim,               "LTRIM$"        },
	{ basic_mid,                 "MID$"          },
	{ basic_netinfo,             "NETINFO$"      },
	{ basic_itoa,                "RADIX$"        },
	{ basic_ramdisk_from_device, "RAMDISK$"      },
	{ basic_ramdisk_from_size,   "RAMDISK"       },
	{ basic_readstring,          "READ$"         },
	{ basic_repeat,              "REP$"          },
	{ basic_reverse,             "REVERSE$"      },
	{ basic_right,               "RIGHT$"        },
	{ basic_rjust,               "RJUST$"        },
	{ basic_rtrim,               "RTRIM$"        },
	{ basic_str,                 "STR$"          },
	{ basic_tokenize,            "TOKENIZE$"     },
	{ basic_trim,                "TRIM$"         },
	{ basic_upper,               "UPPER$"        },
	{ basic_date,                "DATE$"         },
	{ basic_time,                "TIME$"         },
	{ basic_get_upstr,           "UPTIME$"       },
	{ basic_udpread,             "UDPREAD$"      },
	{ basic_udplastip,           "UDPLASTIP$"    },
	{ NULL,                      NULL            },
};

void begin_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx) {
	ctx->bracket_depth = 0;
	ctx->param = def->params;
	ctx->item_begin = (char*)ctx->ptr;
}


uint8_t extract_comma_list(struct ub_proc_fn_def* def, struct basic_ctx* ctx) {
	if (*ctx->ptr == '(') {
		ctx->bracket_depth++;
		if (ctx->bracket_depth == 1) {
			ctx->item_begin = ctx->ptr + 1;
		}
	}
	else if (*ctx->ptr == ')') {
		ctx->bracket_depth--;
	}
	if ((*ctx->ptr == ',' && ctx->bracket_depth == 1) || (*ctx->ptr == ')' && ctx->bracket_depth == 0)) {
		// next item
		// set local vars here
		// Set ctx to item_begin, call expr(), set ctx back again. Change expr to stop on comma.
		//
		// XXX We know wether to call expr or str_expr here based upon the type for the fn param
		// which we will read when this is implemented. We should probably check the fn exists
		// before we even GET here!
		char oldval = *ctx->ptr;
		char oldct = ctx->current_token;
		char* oldptr = (char*)ctx->ptr;
		char* oldnextptr = (char*)ctx->nextptr;
		ctx->nextptr = ctx->item_begin;
		ctx->ptr = ctx->item_begin;
		ctx->current_token = get_next_token(ctx);
		*oldptr = 0;
		if (ctx->param) {
			size_t len = strlen(ctx->param->name);
			if (ctx->param->name[len - 1] == '$') {
				basic_set_string_variable(ctx->param->name, str_expr(ctx), ctx, true, false);
			} else if (ctx->param->name[len - 1] == '#') {
				double f = 0.0;
				double_expr(ctx, &f);
				basic_set_double_variable(ctx->param->name, f, ctx, true, false);
			} else {
				int64_t e = expr(ctx);
				basic_set_int_variable(ctx->param->name, e, ctx, true, false);
			}

			ctx->param = ctx->param->next;
		}
		*oldptr = oldval;
		ctx->ptr = oldptr;
		ctx->nextptr = oldnextptr;
		ctx->current_token = oldct;
		ctx->item_begin = (char*)ctx->ptr + 1;
	}

	ctx->ptr++;

	if (ctx->bracket_depth == 0 || *ctx->ptr == 0) {
		ctx->nextptr = ctx->ptr;
		return 0;
	}

	return 1;

}

const char* basic_eval_str_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	const char* rv = "";
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_STRING;
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			rv = gc_strdup(ctx, (const char*)atomic->fn_return);
		}

		/* Copy list heads back (int/str/double).
		 * Move-to-front inside atomic may change them,
		 * so resync here. Do not free: caller owns.
		 */
		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

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
char basic_builtin_int_fn(const char* fn_name, struct basic_ctx* ctx, int64_t* res) {
	int i;
	for (i = 0; builtin_int[i].name; ++i) {
		if (!strcmp(fn_name, builtin_int[i].name)) {
			*res = builtin_int[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}

char basic_builtin_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res) {
	int i;
	for (i = 0; builtin_double[i].name; ++i) {
		if (!strcmp(fn_name, builtin_double[i].name)) {
			builtin_double[i].handler(ctx, res);
			return 1;
		}
	}
	return 0;
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
char basic_builtin_str_fn(const char* fn_name, struct basic_ctx* ctx, char** res) {
	int i;
	for (i = 0; builtin_str[i].name; ++i) {
		if (!strcmp(fn_name, builtin_str[i].name)) {
			*res = builtin_str[i].handler(ctx);
			return 1;
		}
	}
	return 0;
}


int64_t basic_eval_int_fn(const char* fn_name, struct basic_ctx* ctx)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	int64_t rv = 0;
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_INT;
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		rv = (int64_t)atomic->fn_return;

		/* Copy list heads back (int/str/double).
		 * Move-to-front inside atomic may change them,
		 * so resync here. Do not free: caller owns.
		 */
		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

		return rv;
	}
	tokenizer_error_print(ctx, "No such integer FN");
	return 0;
}

void basic_eval_double_fn(const char* fn_name, struct basic_ctx* ctx, double* res)
{
	struct ub_proc_fn_def* def = basic_find_fn(fn_name + 2, ctx);
	if (def) {
		ctx->call_stack_ptr++;
		init_local_heap(ctx);
		begin_comma_list(def, ctx);
		while (extract_comma_list(def, ctx));
		struct basic_ctx* atomic = basic_clone(ctx);
		atomic->fn_type = RT_FLOAT;
		jump_linenum(def->line, atomic);

		while (!basic_finished(atomic)) {
			line_statement(atomic);
			if (atomic->errored) {
				ctx->errored = true;
				break;
			}
		}
		if (atomic->fn_return == NULL) {
			tokenizer_error_print(ctx, "End of function without returning value");
		} else {
			*res = *((double*)atomic->fn_return);
		}

		/* Copy list heads back (int/str/double).
		 * Move-to-front inside atomic may change them,
		 * so resync here. Do not free: caller owns.
		 */
		ctx->int_variables    = atomic->int_variables;
		ctx->str_variables    = atomic->str_variables;
		ctx->double_variables = atomic->double_variables;

		/* Only free the base struct! */
		buddy_free(ctx->allocator, atomic);

		free_local_heap(ctx);
		ctx->call_stack_ptr--;

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
			strlcpy(linetext, linestart, lineend - linestart + 1);

			char* search = linetext;

			while (*search++ >= '0' && *search <= '9')
				search++;
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

				struct ub_proc_fn_def* def = buddy_malloc(ctx->allocator, sizeof(struct ub_proc_fn_def));
				def->name = buddy_strdup(ctx->allocator, name);
				def->type = type;
				def->line = currentline;
				def->next = ctx->defs;

				/* Parse parameters */

				def->params = NULL;

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

							par->next = NULL;
							par->name = buddy_strdup(ctx->allocator, pname);

							if (def->params == NULL) {
								def->params = par;
							} else {
								struct ub_param* cur = def->params;
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
				ctx->defs = def;
			}

			if (*program == '\n') {
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
	struct ub_proc_fn_def* cur = ctx->defs;
	for (; cur; cur = cur->next) {
		if (!strcmp(name, cur->name)) {
			return cur;
		}
	}
	return NULL;
}

void basic_free_defs(struct basic_ctx* ctx)
{
	for (; ctx->defs; ) {
		for (; ctx->defs->params; ) {
			void* next = ctx->defs->params->next;
			buddy_free(ctx->allocator, ctx->defs->params->name);
			buddy_free(ctx->allocator, ctx->defs->params);
			ctx->defs->params = next;
		}
		void* next = ctx->defs->next;
		buddy_free(ctx->allocator, ctx->defs->name);
		buddy_free(ctx->allocator, ctx->defs);
		ctx->defs = next;
	}
	ctx->defs = NULL;
}

bool is_builtin_double_fn(const char* fn_name) {
	for (int i = 0; builtin_double[i].name; ++i) {
		if (!strcmp(fn_name, builtin_double[i].name)) {
			return true;
		}
	}
	return false;
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
			ctx->call_stack_ptr++;
			init_local_heap(ctx);
			begin_comma_list(def, ctx);
			while (extract_comma_list(def, ctx));
			ctx->call_stack_ptr--;
		} else {
			ctx->call_stack_ptr++;
			init_local_heap(ctx);
			ctx->call_stack_ptr--;
		}

		ctx->fn_type_stack[ctx->call_stack_ptr] = ctx->fn_type; // save caller’s type
		ctx->fn_type = RT_NONE;

		while (tokenizer_token(ctx) != NEWLINE && tokenizer_token(ctx) != ENDOFINPUT) {
			tokenizer_next(ctx);
		}
		accept_or_return(NEWLINE, ctx);

		if (ctx->call_stack_ptr < MAX_CALL_STACK_DEPTH) {
			ctx->call_stack[ctx->call_stack_ptr] = tokenizer_num(ctx, NUMBER);
			basic_debug("PROC from %lu returning line %lu, PROC %s on line %lu\n", ctx->current_linenum, ctx->call_stack[ctx->call_stack_ptr], procname, def->line);
			ctx->call_stack_ptr++;
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
	while (*ctx->nextptr && *ctx->nextptr != '\n') ctx->nextptr++;
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
		ctx->call_stack_ptr--;

		/* Now restore the *caller*'s return type. */
		ctx->fn_type = ctx->fn_type_stack[ctx->call_stack_ptr];

		ctx->if_nest_level = 0; // If we exit a proc, we clear the nest level

		basic_debug("ENDPROC back to line %lu stack pos %lu\n", ctx->call_stack[ctx->call_stack_ptr], ctx->call_stack_ptr);
		jump_linenum(ctx->call_stack[ctx->call_stack_ptr], ctx);
	} else {
		tokenizer_error_print(ctx, "ENDPROC when not inside PROC");
	}
}


