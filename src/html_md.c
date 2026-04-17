#include "html_md.h"
#include <kernel.h>

typedef enum {
	tag_unknown,
	tag_a,
	tag_p,
	tag_br,
	tag_h1,
	tag_h2,
	tag_h3,
	tag_h4,
	tag_h5,
	tag_h6,
	tag_img,
	tag_ul,
	tag_ol,
	tag_li,
	tag_pre,
	tag_code,
	tag_blockquote,
	tag_table,
	tag_tr,
	tag_th,
	tag_td,
	tag_hr,
	tag_head,
	tag_script,
	tag_style,
	tag_template,
	tag_nav,
	tag_noscript,
	tag_link,
	tag_meta,
	tag_title,
	tag_form,
	tag_input,
	tag_textarea,
	tag_select,
	tag_option,
	tag_button
} tag_t;

typedef struct {
	const char *html;
	size_t html_len;
	size_t index;

	char *md;
	size_t md_len;
	size_t md_cap;

	char tag_buf[512];
	size_t tag_len;

	char tag_name[64];

	int in_tag;
	int closing_tag;
	int self_closing_tag;
	int in_attr_value;
	char attr_quote;

	int in_pre;
	int in_code;
	int in_list;
	int in_ol;
	int in_table;
	int in_blockquote;
	int ignore_depth;

	int current_row_is_header;
	int emitted_header_rule;
	int row_col_count;
	int th_count;

	uint8_t list_index;
	uint8_t blockquote_depth;

	size_t line_len;

	char *anchor_href;
	char *anchor_title;

	html2md_options_t opt;

	int suppress_text_depth;
} html2md_ctx_t;

static void md_append_char(html2md_ctx_t *ctx, char ch);
static char md_prev_char(const html2md_ctx_t *ctx);

static void html2md_set_default_options(html2md_options_t *opt)
{
	memset(opt, 0, sizeof(*opt));
	opt->split_lines = true;
	opt->soft_break = 80;
	opt->hard_break = 100;
	opt->unordered_list = '-';
	opt->ordered_list = '.';
	opt->include_title = true;
	opt->format_table = true;
	opt->force_left_trim = false;
	opt->compress_whitespace = false;
	opt->escape_numbered_list = true;
	opt->keep_html_entities = false;
}

static void ensure_line_start(html2md_ctx_t *ctx)
{
	if (ctx->md_len != 0 && md_prev_char(ctx) != '\n') {
		md_append_char(ctx, '\n');
	}
}

static void md_reserve(html2md_ctx_t *ctx, size_t extra)
{
	size_t need = ctx->md_len + extra + 1;

	if (need <= ctx->md_cap) {
		return;
	}

	size_t new_cap = ctx->md_cap ? ctx->md_cap * 2 : 256;

	while (new_cap < need) {
		new_cap *= 2;
	}

	char *new_md = krealloc(ctx->md, new_cap);

	if (!new_md) {
		return;
	}

	ctx->md = new_md;
	ctx->md_cap = new_cap;
}

static void md_append_char(html2md_ctx_t *ctx, char ch)
{
	md_reserve(ctx, 1);
	ctx->md[ctx->md_len++] = ch;

	if (ch == '\n') {
		ctx->line_len = 0;
	} else {
		ctx->line_len++;
	}
}

static void md_append_mem(html2md_ctx_t *ctx, const char *s, size_t len)
{
	md_reserve(ctx, len);
	memcpy(ctx->md + ctx->md_len, s, len);
	ctx->md_len += len;

	for (size_t i = 0; i < len; i++) {
		if (s[i] == '\n') {
			ctx->line_len = 0;
		} else {
			ctx->line_len++;
		}
	}
}

static void md_append_str(html2md_ctx_t *ctx, const char *s)
{
	md_append_mem(ctx, s, strlen(s));
}

static void md_append_repeat(html2md_ctx_t *ctx, const char *s, size_t count)
{
	for (size_t i = 0; i < count; i++) {
		md_append_str(ctx, s);
	}
}

static char md_prev_char(const html2md_ctx_t *ctx)
{
	if (ctx->md_len == 0) {
		return 0;
	}

	return ctx->md[ctx->md_len - 1];
}

static char md_prev_prev_char(const html2md_ctx_t *ctx)
{
	if (ctx->md_len < 2) {
		return 0;
	}

	return ctx->md[ctx->md_len - 2];
}

#define streq(a, b) (strcmp((a), (b)) == 0)

static int starts_with(const char *a, const char *b)
{
	while (*b) {
		if (*a != *b) {
			return 0;
		}
		a++;
		b++;
	}

	return 1;
}

static tag_t tag_from_name(const char *name)
{
	if (streq(name, "a")) {
		return tag_a;
	}
	if (streq(name, "p")) {
		return tag_p;
	}
	if (streq(name, "br")) {
		return tag_br;
	}
	if (streq(name, "h1")) {
		return tag_h1;
	}
	if (streq(name, "h2")) {
		return tag_h2;
	}
	if (streq(name, "h3")) {
		return tag_h3;
	}
	if (streq(name, "h4")) {
		return tag_h4;
	}
	if (streq(name, "h5")) {
		return tag_h5;
	}
	if (streq(name, "h6")) {
		return tag_h6;
	}
	if (streq(name, "img")) {
		return tag_img;
	}
	if (streq(name, "ul")) {
		return tag_ul;
	}
	if (streq(name, "ol")) {
		return tag_ol;
	}
	if (streq(name, "li")) {
		return tag_li;
	}
	if (streq(name, "pre")) {
		return tag_pre;
	}
	if (streq(name, "code")) {
		return tag_code;
	}
	if (streq(name, "blockquote")) {
		return tag_blockquote;
	}
	if (streq(name, "table")) {
		return tag_table;
	}
	if (streq(name, "tr")) {
		return tag_tr;
	}
	if (streq(name, "th")) {
		return tag_th;
	}
	if (streq(name, "td")) {
		return tag_td;
	}
	if (streq(name, "hr")) {
		return tag_hr;
	}
	if (streq(name, "head")) {
		return tag_head;
	}
	if (streq(name, "script")) {
		return tag_script;
	}
	if (streq(name, "style")) {
		return tag_style;
	}
	if (streq(name, "template")) {
		return tag_template;
	}
	if (streq(name, "nav")) {
		return tag_nav;
	}
	if (streq(name, "noscript")) {
		return tag_noscript;
	}
	if (streq(name, "link")) {
		return tag_link;
	}
	if (streq(name, "meta")) {
		return tag_meta;
	}
	if (streq(name, "title")) {
		return tag_title;
	}
	if (streq(name, "form")) {
		return tag_form;
	}
	if (streq(name, "input")) {
		return tag_input;
	}
	if (streq(name, "textarea")) {
		return tag_textarea;
	}
	if (streq(name, "select")) {
		return tag_select;
	}
	if (streq(name, "option")) {
		return tag_option;
	}
	if (streq(name, "button")) {
		return tag_button;
	}
	return tag_unknown;
}

static void md_append_attr_quoted(html2md_ctx_t *ctx, const char *key, const char *value)
{
	if (!value || !*value) {
		return;
	}

	md_append_char(ctx, ' ');
	md_append_str(ctx, key);
	md_append_str(ctx, "=\"");
	md_append_str(ctx, value);
	md_append_char(ctx, '"');
}

static void md_append_attr_plain(html2md_ctx_t *ctx, const char *key, const char *value)
{
	if (!value || !*value) {
		return;
	}

	md_append_char(ctx, ' ');
	md_append_str(ctx, key);
	md_append_char(ctx, '=');
	md_append_str(ctx, value);
}

static void md_append_flag(html2md_ctx_t *ctx, const char *flag, int enabled)
{
	if (!enabled) {
		return;
	}

	md_append_char(ctx, ' ');
	md_append_str(ctx, flag);
}

static int has_attr(html2md_ctx_t *ctx, const char *name)
{
	const char *p = ctx->tag_buf;
	size_t name_len = strlen(name);

	while (*p) {
		while (*p && isspace((unsigned char)*p)) {
			p++;
		}

		if (!*p) {
			break;
		}

		if (starts_with(p, name)) {
			const char *q = p + name_len;

			if (*q == 0 || isspace((unsigned char)*q) || *q == '=' || *q == '/' || *q == '>') {
				return 1;
			}
		}

		p++;
	}

	return 0;
}

static int tag_is_ignored(tag_t tag, const html2md_options_t *opt)
{
	if (tag == tag_title) {
		return !opt->include_title;
	}

	return tag == tag_head || tag == tag_script || tag == tag_style ||
		   tag == tag_template || tag == tag_nav || tag == tag_noscript ||
		   tag == tag_link || tag == tag_meta;
}

static void extract_tag_name(const char *tag_buf, char *out, size_t out_size)
{
	size_t i = 0;
	size_t j = 0;

	while (tag_buf[i] && isspace((unsigned char)tag_buf[i])) {
		i++;
	}

	if (tag_buf[i] == '/') {
		i++;
	}

	while (tag_buf[i] && !isspace((unsigned char)tag_buf[i]) &&
		   tag_buf[i] != '/' && j + 1 < out_size) {
		out[j++] = tag_buf[i++];
	}

	out[j] = 0;
}

static char *extract_attr(html2md_ctx_t *ctx, const char *name)
{
	const char *p = ctx->tag_buf;
	size_t name_len = strlen(name);

	while (*p) {
		while (*p && isspace((unsigned char)*p)) {
			p++;
		}

		if (!*p) {
			break;
		}

		if (strncmp(p, name, name_len) == 0) {
			char next = p[name_len];

			if (next == '=' || isspace((unsigned char)next)) {
				const char *q = p + name_len;

				while (*q && isspace((unsigned char)*q)) {
					q++;
				}

				if (*q != '=') {
					p++;
					continue;
				}

				q++;

				while (*q && isspace((unsigned char)*q)) {
					q++;
				}

				if (*q == '"' || *q == '\'') {
					char quote = *q++;
					const char *start = q;

					while (*q && *q != quote) {
						q++;
					}

					size_t len = (size_t)(q - start);
					char *out = kmalloc(len + 1);

					if (!out) {
						return NULL;
					}

					memcpy(out, start, len);
					out[len] = 0;
					return out;
				}

				{
					const char *start = q;

					while (*q && !isspace((unsigned char)*q) && *q != '/' && *q != '>') {
						q++;
					}

					size_t len = (size_t)(q - start);
					char *out = kmalloc(len + 1);

					if (!out) {
						return NULL;
					}

					memcpy(out, start, len);
					out[len] = 0;
					return out;
				}
			}
		}

		while (*p && !isspace((unsigned char)*p)) {
			p++;
		}
	}

	return NULL;
}

static void append_blockquote_prefix(html2md_ctx_t *ctx)
{
	if (ctx->blockquote_depth == 0) {
		return;
	}

	md_append_repeat(ctx, "> ", ctx->blockquote_depth);
}

static void ensure_blank_line(html2md_ctx_t *ctx)
{
	if (ctx->md_len == 0) {
		return;
	}

	if (md_prev_char(ctx) != '\n') {
		md_append_char(ctx, '\n');
	}

	if (md_prev_prev_char(ctx) != '\n') {
		md_append_char(ctx, '\n');
	}
}

static int decode_entity(const char *s, size_t *consumed, const char **replacement)
{
	static const struct {
		const char *entity;
		const char *replacement;
	} map[] = {
		{ "&quot;", "\"" },
		{ "&lt;", "<" },
		{ "&gt;", ">" },
		{ "&amp;", "&" },
		{ "&nbsp;", " " },
		{ "&ndash;", "-" },
		{ "&hellip;", "..." },
		{ "&le;", "<=" },
		{ "&ge;", ">=" },
		{ "&ne;", "!=" },		
		{ "&apos;", "'" },
		{ "&ldquo;", "\"" },
		{ "&rdquo;", "\"" },
		{ "&lsquo;", "'" },
		{ "&rsquo;", "'" },
		{ "&frac12;", "1/2" },
		{ "&frac14;", "1/4" },
		{ "&frac34;", "3/4" },
		{ "&plusmn;", "+/-" },		
		{ "&larr;", GLYPH_LARR },
		{ "&rarr;", GLYPH_RARR },
		{ "&uarr;", GLYPH_UARR },
		{ "&darr;", GLYPH_DARR },
		{ "&copy;", GLYPH_COPY },
		{ "&reg;", GLYPH_REG },
		{ "&trade;", GLYPH_TRADE },
		{ "&deg;", GLYPH_DEG },
		{ "&mdash;", GLYPH_EMDASH },
	};

	for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
		size_t len = strlen(map[i].entity);

		if (strncmp(s, map[i].entity, len) == 0) {
			*consumed = len;
			*replacement = map[i].replacement;
			return 1;
		}
	}

	return 0;
}

static void open_tag(html2md_ctx_t *ctx, tag_t tag)
{
	if (tag_is_ignored(tag, &ctx->opt)) {
		ctx->ignore_depth++;
		return;
	}

	if (ctx->ignore_depth != 0) {
		return;
	}

	switch (tag) {
	case tag_a:
		md_append_char(ctx, '[');
		ctx->anchor_href = extract_attr(ctx, "href");
		ctx->anchor_title = extract_attr(ctx, "title");
		break;

	case tag_img: {
		char *alt = extract_attr(ctx, "alt");
		char *src = extract_attr(ctx, "src");
		char *title = extract_attr(ctx, "title");

		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "![");
		if (alt) {
			md_append_str(ctx, alt);
		}
		md_append_str(ctx, "](");
		if (src) {
			md_append_str(ctx, src);
		}
		if (title && *title) {
			md_append_str(ctx, " \"");
			md_append_str(ctx, title);
			md_append_char(ctx, '"');
		}
		md_append_char(ctx, ')');

		if (alt) {
			kfree(alt);
		}
		if (src) {
			kfree(src);
		}
		if (title) {
			kfree(title);
		}
		break;
	}

	case tag_p:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		break;

	case tag_br:
		if (ctx->in_table) {
			md_append_str(ctx, "<br>");
		} else {
			md_append_str(ctx, "  \n");
			append_blockquote_prefix(ctx);
		}
		break;

	case tag_h1:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "# ");
		break;

	case tag_h2:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "## ");
		break;

	case tag_h3:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "### ");
		break;

	case tag_h4:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "#### ");
		break;

	case tag_h5:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "##### ");
		break;

	case tag_h6:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "###### ");
		break;

	case tag_ul:
		ctx->in_list = 1;
		break;

	case tag_ol:
		ctx->in_list = 1;
		ctx->in_ol = 1;
		ctx->list_index = 0;
		break;

	case tag_li:
		if (md_prev_char(ctx) != '\n') {
			md_append_char(ctx, '\n');
		}
		append_blockquote_prefix(ctx);

		if (ctx->in_ol) {
			char num[16];
			size_t n = 0;
			unsigned value = ++ctx->list_index;
			char rev[16];
			size_t r = 0;

			do {
				rev[r++] = (char)('0' + (value % 10));
				value /= 10;
			} while (value != 0);

			while (r != 0) {
				num[n++] = rev[--r];
			}

			num[n++] = ctx->opt.ordered_list;
			num[n++] = ' ';
			num[n] = 0;
			md_append_str(ctx, num);
		} else {
			char bullet[3];
			bullet[0] = ctx->opt.unordered_list;
			bullet[1] = ' ';
			bullet[2] = 0;
			md_append_str(ctx, bullet);
		}
		break;

	case tag_pre:
		ctx->in_pre = 1;
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "```");
		break;

	case tag_code:
		if (!ctx->in_pre) {
			ctx->in_code = 1;
			md_append_char(ctx, '`');
		}
		break;

	case tag_blockquote:
		ctx->in_blockquote = 1;
		ctx->blockquote_depth++;
		ensure_blank_line(ctx);
		break;

	case tag_table:
		ctx->in_table = 1;
		ctx->current_row_is_header = 0;
		ctx->emitted_header_rule = 0;
		ensure_blank_line(ctx);
		break;

	case tag_tr:
		ctx->row_col_count = 0;
		ctx->th_count = 0;
		break;

	case tag_th:
		if (ctx->row_col_count == 0) {
			md_append_str(ctx, "| ");
		} else {
			md_append_str(ctx, " | ");
		}
		ctx->row_col_count++;
		ctx->th_count++;
		ctx->current_row_is_header = 1;
		break;

	case tag_td:
		if (ctx->row_col_count == 0) {
			md_append_str(ctx, "| ");
		} else {
			md_append_str(ctx, " | ");
		}
		ctx->row_col_count++;
		break;

	case tag_hr:
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		md_append_str(ctx, "---");
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		break;

	case tag_form: {
		char *method = extract_attr(ctx, "method");
		char *action = extract_attr(ctx, "action");

		ensure_blank_line(ctx);
		md_append_str(ctx, "#form begin");

		if (method && *method) {
			md_append_attr_plain(ctx, "method", method);
		} else {
			md_append_str(ctx, " method=get");
		}

		if (action && *action) {
			md_append_attr_quoted(ctx, "action", action);
		}

		md_append_char(ctx, '\n');

		if (method) {
			kfree(method);
		}
		if (action) {
			kfree(action);
		}
		break;
	}
	case tag_input: {
		char *name = extract_attr(ctx, "name");
		char *type = extract_attr(ctx, "type");
		char *value = extract_attr(ctx, "value");
		char *placeholder = extract_attr(ctx, "placeholder");

		ensure_line_start(ctx);
		md_append_str(ctx, "#form input");
		md_append_attr_plain(ctx, "name", name);

		if (type && *type) {
			md_append_attr_plain(ctx, "type", type);
		} else {
			md_append_str(ctx, " type=text");
		}

		md_append_attr_quoted(ctx, "value", value);
		md_append_attr_quoted(ctx, "placeholder", placeholder);
		md_append_flag(ctx, "checked", has_attr(ctx, "checked"));
		md_append_flag(ctx, "selected", has_attr(ctx, "selected"));
		md_append_flag(ctx, "disabled", has_attr(ctx, "disabled"));
		md_append_flag(ctx, "readonly", has_attr(ctx, "readonly"));
		md_append_char(ctx, '\n');

		if (name) {
			kfree(name);
		}
		if (type) {
			kfree(type);
		}
		if (value) {
			kfree(value);
		}
		if (placeholder) {
			kfree(placeholder);
		}
		break;
	}
	case tag_textarea: {
		char *name = extract_attr(ctx, "name");
		char *rows = extract_attr(ctx, "rows");
		char *cols = extract_attr(ctx, "cols");
		ctx->suppress_text_depth++;
		ensure_line_start(ctx);
		md_append_str(ctx, "#form textarea");
		md_append_attr_plain(ctx, "name", name);
		md_append_attr_plain(ctx, "rows", rows);
		md_append_attr_plain(ctx, "cols", cols);
		md_append_char(ctx, '\n');

		if (name) {
			kfree(name);
		}
		if (rows) {
			kfree(rows);
		}
		if (cols) {
			kfree(cols);
		}
		break;
	}
	case tag_select: {
		char *name = extract_attr(ctx, "name");
		ensure_line_start(ctx);
		md_append_str(ctx, "#form select");
		md_append_attr_plain(ctx, "name", name);
		md_append_flag(ctx, "disabled", has_attr(ctx, "disabled"));
		md_append_char(ctx, '\n');

		if (name) {
			kfree(name);
		}
		break;
	}
	case tag_option: {
		char *value = extract_attr(ctx, "value");
		ctx->suppress_text_depth++;
		ensure_line_start(ctx);
		md_append_str(ctx, "#form option");
		md_append_attr_plain(ctx, "value", value);
		md_append_flag(ctx, "selected", has_attr(ctx, "selected"));
		md_append_flag(ctx, "disabled", has_attr(ctx, "disabled"));
		md_append_char(ctx, '\n');

		if (value) {
			kfree(value);
		}
		break;
	}
	case tag_button: {
		ctx->suppress_text_depth++;
		char *type = extract_attr(ctx, "type");
		char *name = extract_attr(ctx, "name");
		char *value = extract_attr(ctx, "value");

		ensure_line_start(ctx);
		md_append_str(ctx, "#form button");

		if (type && *type) {
			md_append_attr_plain(ctx, "type", type);
		} else {
			md_append_str(ctx, " type=submit");
		}

		md_append_attr_plain(ctx, "name", name);
		md_append_attr_quoted(ctx, "value", value);
		md_append_flag(ctx, "disabled", has_attr(ctx, "disabled"));
		md_append_char(ctx, '\n');

		if (type) {
			kfree(type);
		}
		if (name) {
			kfree(name);
		}
		if (value) {
			kfree(value);
		}
		break;
	}
	default:
		break;
	}
}

static void close_tag(html2md_ctx_t *ctx, tag_t tag)
{
	if (tag_is_ignored(tag, &ctx->opt)) {
		if (ctx->ignore_depth != 0) {
			ctx->ignore_depth--;
		}
		return;
	}

	if (ctx->ignore_depth != 0) {
		return;
	}

	switch (tag) {

	case tag_a:
		md_append_str(ctx, "](");

		if (ctx->anchor_href) {
			md_append_str(ctx, ctx->anchor_href);
		}

		if (ctx->anchor_title && *ctx->anchor_title) {
			md_append_str(ctx, " \"");
			md_append_str(ctx, ctx->anchor_title);
			md_append_char(ctx, '"');
		}

		md_append_char(ctx, ')');

		if (ctx->anchor_href) {
			kfree(ctx->anchor_href);
			ctx->anchor_href = NULL;
		}

		if (ctx->anchor_title) {
			kfree(ctx->anchor_title);
			ctx->anchor_title = NULL;
		}
		break;

	case tag_p:
	case tag_h1:
	case tag_h2:
	case tag_h3:
	case tag_h4:
	case tag_h5:
	case tag_h6:
		if (md_prev_char(ctx) != '\n') {
			md_append_char(ctx, '\n');
		}
		break;

	case tag_ul:
	case tag_ol:
		ctx->in_list = 0;
		ctx->in_ol = 0;
		ensure_blank_line(ctx);
		append_blockquote_prefix(ctx);
		break;

	case tag_li:
		if (md_prev_char(ctx) != '\n') {
			md_append_char(ctx, '\n');
		}
		break;

	case tag_pre:
		ctx->in_pre = 0;
		md_append_str(ctx, "```");
		md_append_char(ctx, '\n');
		break;

	case tag_code:
		if (!ctx->in_pre) {
			ctx->in_code = 0;
			md_append_char(ctx, '`');
		}
		break;

	case tag_blockquote:
		if (ctx->blockquote_depth != 0) {
			ctx->blockquote_depth--;
		}
		if (ctx->blockquote_depth == 0) {
			ctx->in_blockquote = 0;
			ensure_blank_line(ctx);
		}
		break;

	case tag_table:
		ctx->in_table = 0;
		ensure_blank_line(ctx);
		break;

	case tag_tr:
		if (ctx->row_col_count != 0) {
			md_append_str(ctx, " |");
			md_append_char(ctx, '\n');

			if (ctx->current_row_is_header && !ctx->emitted_header_rule) {
				for (int i = 0; i < ctx->row_col_count; i++) {
					md_append_str(ctx, "| --- ");
				}
				md_append_str(ctx, "|");
				md_append_char(ctx, '\n');
				ctx->emitted_header_rule = 1;
			}
		}

		ctx->row_col_count = 0;
		ctx->th_count = 0;
		ctx->current_row_is_header = 0;
		break;

	case tag_form:
		ensure_line_start(ctx);
		md_append_str(ctx, "#form end\n");
		break;

	case tag_select:
		ensure_line_start(ctx);
		md_append_str(ctx, "#form endselect\n");
		break;

	case tag_textarea:
	case tag_option:
	case tag_button:
		if (ctx->suppress_text_depth != 0) {
			ctx->suppress_text_depth--;
		}
		break;

	default:
		break;
	}
}

static void finish_tag(html2md_ctx_t *ctx)
{
	ctx->tag_buf[ctx->tag_len] = 0;
	extract_tag_name(ctx->tag_buf, ctx->tag_name, sizeof(ctx->tag_name));

	if (ctx->tag_name[0] == 0) {
		return;
	}

	tag_t tag = tag_from_name(ctx->tag_name);

	if (!ctx->closing_tag) {
		open_tag(ctx, tag);
	} else {
		close_tag(ctx, tag);
	}
}

static void parse_tag_char(html2md_ctx_t *ctx, char ch)
{
	if (ch == '"' || ch == '\'') {
		if (ctx->in_attr_value && ch == ctx->attr_quote) {
			ctx->in_attr_value = 0;
			ctx->attr_quote = 0;
		} else if (!ctx->in_attr_value) {
			ctx->in_attr_value = 1;
			ctx->attr_quote = ch;
		}
	}

	if (!ctx->in_attr_value && ch == '/' && ctx->tag_len == 0) {
		ctx->closing_tag = 1;
		return;
	}

	if (!ctx->in_attr_value && ch == '>') {
		ctx->in_tag = 0;
		finish_tag(ctx);
		return;
	}

	if (!ctx->in_attr_value && ch == '/' && ctx->tag_len != 0) {
		ctx->self_closing_tag = 1;
	}

	if (ctx->tag_len + 1 < sizeof(ctx->tag_buf)) {
		ctx->tag_buf[ctx->tag_len++] = (char)tolower((unsigned char)ch);
	}
}

static void parse_text_char(html2md_ctx_t *ctx, char ch)
{
	if (ctx->ignore_depth != 0) {
		return;
	}

	if (!ctx->opt.keep_html_entities && ch == '&') {
		size_t consumed = 0;
		const char *replacement = NULL;

		if (decode_entity(ctx->html + ctx->index, &consumed, &replacement)) {
			md_append_str(ctx, replacement);
			ctx->index += consumed - 1;
			return;
		}
	}

	if (!ctx->in_pre && ch == '\r') {
		return;
	}

	if (!ctx->in_pre && ch == '\n') {
		if (ctx->in_blockquote) {
			md_append_char(ctx, '\n');
			append_blockquote_prefix(ctx);
		} else if (!ctx->opt.compress_whitespace) {
			md_append_char(ctx, '\n');
		} else if (md_prev_char(ctx) != ' ' && md_prev_char(ctx) != '\n') {
			md_append_char(ctx, ' ');
		}
		return;
	}

	if (!ctx->in_pre && ctx->opt.compress_whitespace && isspace((unsigned char)ch)) {
		if (md_prev_char(ctx) != ' ' && md_prev_char(ctx) != '\n') {
			md_append_char(ctx, ' ');
		}
		return;
	}

	if (!ctx->in_pre) {
		if (ch == '*') {
			md_append_str(ctx, "\\*");
			return;
		}
		if (ch == '`') {
			md_append_str(ctx, "\\`");
			return;
		}
		if (ch == '\\') {
			md_append_str(ctx, "\\\\");
			return;
		}
	}

	md_append_char(ctx, ch);
}

static void tidy_markdown(html2md_ctx_t *ctx)
{
	char *src = ctx->md;
	size_t src_len = ctx->md_len;
	size_t read = 0;
	size_t write = 0;
	int newline_run = 0;

	while (read < src_len) {
		char ch = src[read++];

		if (ch == '\n') {
			newline_run++;
			if (newline_run <= 2) {
				src[write++] = ch;
			}
			continue;
		}

		newline_run = 0;
		src[write++] = ch;
	}

	while (write != 0 && (src[write - 1] == '\n' || src[write - 1] == ' ')) {
		write--;
	}

	ctx->md_len = write;
	ctx->md[ctx->md_len] = 0;
}

bool html2md_convert(const char *html, const html2md_options_t *options, html2md_result_t *out)
{
	html2md_ctx_t ctx;

	memset(&ctx, 0, sizeof(ctx));

	if (!html || !out) {
		return false;
	}

	if (options) {
		ctx.opt = *options;
	} else {
		html2md_set_default_options(&ctx.opt);
	}

	ctx.html = html;
	ctx.html_len = strlen(html);

	for (ctx.index = 0; ctx.index < ctx.html_len; ctx.index++) {
		char ch = ctx.html[ctx.index];

		if (!ctx.in_tag && ch == '<') {
			ctx.in_tag = 1;
			ctx.closing_tag = 0;
			ctx.self_closing_tag = 0;
			ctx.in_attr_value = 0;
			ctx.attr_quote = 0;
			ctx.tag_len = 0;
			continue;
		}

		if (ctx.in_tag) {
			parse_tag_char(&ctx, ch);
		} else {
			parse_text_char(&ctx, ch);
		}
	}

	if (!ctx.md) {
		out->markdown = NULL;
		out->length = 0;
		out->ok = false;
		return false;
	}

	ctx.md[ctx.md_len] = 0;
	tidy_markdown(&ctx);

	out->markdown = ctx.md;
	out->length = ctx.md_len;
	out->ok = ctx.in_tag == 0 && ctx.ignore_depth == 0 && ctx.in_pre == 0 &&
			  ctx.in_code == 0 && ctx.in_table == 0;

	if (ctx.anchor_href) {
		kfree(ctx.anchor_href);
	}
	if (ctx.anchor_title) {
		kfree(ctx.anchor_title);
	}

	return true;
}

void html2md_free(html2md_result_t *res)
{
	if (!res) {
		return;
	}

	if (res->markdown) {
		kfree(res->markdown);
		res->markdown = NULL;
	}

	res->length = 0;
	res->ok = false;
}


void html2md_define_glyphs(void)
{
	redefine_character(*GLYPH_LARR, (uint8_t[8]){ 0x08, 0x10, 0x20, 0x7F, 0x20, 0x10, 0x08, 0x00 });
	redefine_character(*GLYPH_RARR, (uint8_t[8]){ 0x10, 0x08, 0x04, 0x7F, 0x04, 0x08, 0x10, 0x00 });
	redefine_character(*GLYPH_UARR, (uint8_t[8]){ 0x10, 0x38, 0x54, 0x10, 0x10, 0x10, 0x10, 0x00 });
	redefine_character(*GLYPH_DARR, (uint8_t[8]){ 0x10, 0x10, 0x10, 0x10, 0x54, 0x38, 0x10, 0x00 });
	redefine_character(*GLYPH_COPY, (uint8_t[8]){ 0x3C, 0x42, 0x9D, 0xA1, 0xA1, 0x9D, 0x42, 0x3C });
	redefine_character(*GLYPH_REG, (uint8_t[8]){ 0x3C, 0x42, 0x9D, 0xA5, 0xB9, 0x85, 0x42, 0x3C });
	redefine_character(*GLYPH_TRADE, (uint8_t[8]){ 0x7F, 0x49, 0x49, 0x7F, 0x00, 0x7F, 0x49, 0x49 });
	redefine_character(*GLYPH_DEG, (uint8_t[8]){ 0x18, 0x24, 0x24, 0x18, 0x00, 0x00, 0x00, 0x00 });
	redefine_character(*GLYPH_EMDASH, (uint8_t[8]){ 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00, 0x00 });
}

// ----------- TEST SHIM --------------

bool html2md_self_test(void)
{
	static const char *html =
		"<head><title>Hidden title</title><script>evil()</script></head>"
		"<h1>Example &amp; test</h1>"
		"<p>Hello <a href=\"https://example.com\" title=\"site\">world</a><br>next line</p>"
		"<blockquote><p>Quoted &rarr; text</p></blockquote>"
		"<ul><li>One</li><li>Two</li></ul>"
		"<p><code>x = 1;</code></p>"
		"<img src=\"/img/logo.png\" alt=\"logo\">"
		"<table>"
		"<tr><th>Name</th><th>Value</th></tr>"
		"<tr><td>alpha</td><td>1</td></tr>"
		"<tr><td>beta</td><td>2</td></tr>"
		"</table>";

	static const char *expected =
		"# Example & test\n"
		"\n"
		"Hello [world](https://example.com \"site\")  \n"
		"next line\n"
		"\n"
		"> Quoted " GLYPH_RARR " text\n"
		"\n"
		"- One\n"
		"- Two\n"
		"\n"
		"`x = 1;`\n"
		"\n"
		"![logo](/img/logo.png)\n"
		"\n"
		"| Name | Value |\n"
		"| --- | --- |\n"
		"| alpha | 1 |\n"
		"| beta | 2 |";

	html2md_result_t result;
	bool ok;

	memset(&result, 0, sizeof(result));

	ok = html2md_convert(html, NULL, &result);

	if (!ok) {
		dprintf("html2md_self_test: html2md_convert returned false\n");
		return false;
	}

	if (!result.markdown) {
		dprintf("html2md_self_test: result.markdown is NULL\n");
		return false;
	}

	if (!result.ok) {
		dprintf("html2md_self_test: result.ok is false\n");
		dprintf("generated markdown was:\n%s\n", result.markdown);
		html2md_free(&result);
		return false;
	}

	if (strcmp(result.markdown, expected) != 0) {
		dprintf("html2md_self_test: markdown mismatch\n");
		dprintf("expected:\n----\n%s\n----\n", expected);
		dprintf("got:\n----\n%s\n----\n", result.markdown);
		html2md_free(&result);
		return false;
	}

	dprintf("html2md_self_test: passed\n");
	html2md_free(&result);
	return true;
}

