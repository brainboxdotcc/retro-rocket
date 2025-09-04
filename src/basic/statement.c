#include <kernel.h>

extern bool debug;

void statement(struct basic_ctx* ctx)
{
	enum token_t token = tokenizer_token(ctx);

	basic_debug("line %ld statement(%d)\n", ctx->current_linenum, token);

	switch (token) {
		case REM:
			return rem_statement(ctx);
		case BACKGROUND:
			return background_statement(ctx);
		case COLOR:
			return colour_statement(ctx, COLOR);
		case COLOUR:
			return colour_statement(ctx, COLOUR);
		case DEF:
			return def_statement(ctx);
		case CHAIN:
			return chain_statement(ctx);
		case EVAL:
			return eval_statement(ctx);
		case DATA:
			return data_statement(ctx);
		case RESTORE:
			return restore_statement(ctx);
		case CLOSE:
			return close_statement(ctx);
		case PRINT:
			return print_statement(ctx);
		case PROC:
			return proc_statement(ctx);
		case ENDPROC:
			return endproc_statement(ctx);
		case IF:
			return if_statement(ctx);
		case ELSE:
			return else_statement(ctx);
		case CURSOR:
			return gotoxy_statement(ctx);
		case GOTO:
			return goto_statement(ctx);
		case GOSUB:
			return gosub_statement(ctx);
		case REPEAT:
			return repeat_statement(ctx);
		case UNTIL:
			return until_statement(ctx);
		case WHILE:
			return while_statement(ctx);
		case ENDWHILE:
			return endwhile_statement(ctx);
		case CONTINUE:
			return continue_statement(ctx);
		case RETURN:
			return return_statement(ctx);
		case FOR:
			return for_statement(ctx);
		case NEXT:
			return next_statement(ctx);
		case END:
			return end_statement(ctx);
		case POKE:
			return poke_statement(ctx);
		case POKEW:
			return pokew_statement(ctx);
		case POKED:
			return poked_statement(ctx);
		case POKEQ:
			return pokeq_statement(ctx);
		case ENDIF:
			return endif_statement(ctx);
		case INPUT:
			return input_statement(ctx);
		case SOCKREAD:
			return sockread_statement(ctx);
		case SOCKWRITE:
			return sockwrite_statement(ctx);
		case DELETE:
			return delete_statement(ctx);
		case RMDIR:
			return rmdir_statement(ctx);
		case MOUNT:
			return mount_statement(ctx);
		case MKDIR:
			return mkdir_statement(ctx);
		case CONNECT:
			return connect_statement(ctx);
		case SOCKCLOSE:
			return sockclose_statement(ctx);
		case OUTPORT:
			return outport_statement(ctx);
		case OUTPORTW:
			return outportw_statement(ctx);
		case OUTPORTD:
			return outportd_statement(ctx);
		case POINT:
			return point_statement(ctx);
		case CLS:
			return cls_statement(ctx);
		case GCOL:
			return gcol_statement(ctx);
		case LINE:
			return draw_line_statement(ctx);
		case TRIANGLE:
			return triangle_statement(ctx);
		case WRITE:
			return write_statement(ctx);
		case RECTANGLE:
			return rectangle_statement(ctx);
		case DIM:
			return dim_statement(ctx);
		case REDIM:
			return redim_statement(ctx);
		case PUSH:
			return push_statement(ctx);
		case POP:
			return pop_statement(ctx);
		case CHDIR:
			return chdir_statement(ctx);
		case CIRCLE:
			return circle_statement(ctx);
		case LIBRARY:
			return library_statement(ctx);
		case YIELD:
			return yield_statement(ctx);
		case SETVARI:
			return setvari_statement(ctx);
		case SETVARR:
			return setvarr_statement(ctx);
		case SETVARS:
			return setvars_statement(ctx);
		case PLOT:
			return plot_statement(ctx);
		case PLOTQUAD:
			return plotquad_statement(ctx);
		case AUTOFLIP:
			return autoflip_statement(ctx);
		case FLIP:
			return flip_statement(ctx);
		case KEYMAP:
			return keymap_statement(ctx);
		case SETTIMEZONE:
			return settimezone_statement(ctx);
		case KGET:
			return kget_statement(ctx);
		case SPRITELOAD:
			return loadsprite_statement(ctx);
		case SPRITEFREE:
			return freesprite_statement(ctx);
		case LET:
			accept_or_return(LET, ctx);
			/* Fall through. */
		case VARIABLE:
			return let_statement(ctx, false, false);
		case GLOBAL:
			accept_or_return(GLOBAL, ctx);
			return let_statement(ctx, true, false);
		case LOCAL:
			accept_or_return(LOCAL, ctx);
			return let_statement(ctx, false, true);
		case EQUALS:
			return eq_statement(ctx);
		case ERROR:
			return error_statement(ctx);
		case ON:
			return on_statement(ctx);
		case OFF:
			return off_statement(ctx);
		case SLEEP:
			return sleep_statement(ctx);
		case UDPWRITE:
			return udpwrite_statement(ctx);
		case UDPBIND:
			return udpbind_statement(ctx);
		case UDPUNBIND:
			return udpunbind_statement(ctx);
		case NEWLINE:
			/* Blank trailing line at end of program, ignore */
			return;
		default:
			dprintf("Unknown keyword: %d\n", token);
			return tokenizer_error_print(ctx, "Unknown keyword");
	}
}

void line_statement(struct basic_ctx* ctx)
{
	basic_debug("line_statement\n");
	if (tokenizer_token(ctx) == NEWLINE) {
		/* Empty line! */
		accept(NEWLINE, ctx);
		return;
	}
	int64_t line = tokenizer_num(ctx, NUMBER);
	basic_debug("line_statement parsed line %ld\n", line);
	if (line == 0) {
		return tokenizer_error_printf(ctx, "Missing line number after line %lu", ctx->current_linenum);
	}
	ctx->current_linenum = line;
	accept_or_return(NUMBER, ctx);
	statement(ctx);
}
