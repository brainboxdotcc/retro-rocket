#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define GLYPH_LARR	"\x80"
#define GLYPH_RARR	"\x81"
#define GLYPH_UARR	"\x82"
#define GLYPH_DARR	"\x83"
#define GLYPH_COPY	"\x84"
#define GLYPH_REG	"\x85"
#define GLYPH_TRADE	"\x86"
#define GLYPH_DEG	"\x87"
#define GLYPH_EMDASH	"\x88"

typedef struct html2md_options {
	bool split_lines;
	int soft_break;
	int hard_break;
	char unordered_list;
	char ordered_list;
	bool include_title;
	bool format_table;
	bool force_left_trim;
	bool compress_whitespace;
	bool escape_numbered_list;
	bool keep_html_entities;
} html2md_options_t;

typedef struct html2md_result {
	char *markdown;
	size_t length;
	bool ok;
} html2md_result_t;

bool html2md_convert(const char *html, const html2md_options_t *options, html2md_result_t *out);
void html2md_free(html2md_result_t *result);
void html2md_define_glyphs(void);
bool html2md_self_test(void);

