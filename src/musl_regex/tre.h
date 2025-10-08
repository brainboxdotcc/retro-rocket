/*
  tre-internal.h - TRE internal definitions

  Copyright (c) 2001-2009 Ville Laurikari <vl@iki.fi>
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.

    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
  HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TRE_INTERNAL_H_
#define TRE_INTERNAL_H_

#include <kernel.h>

#undef  TRE_MBSTATE
#define NDEBUG

#define TRE_REGEX_T_FIELD __opaque
typedef int reg_errcode_t;

/* --- Core character type: wide, as original TRE expects --- */
typedef wchar_t tre_char_t;

/* bridge macro used in some TRE codepaths */
#define tre_mbrtowc(pwc, s, n, ps) (mbtowc((pwc), (s), (n)))

/* comparisons/limits in wide domain */
typedef int   tre_cint_t;
#define TRE_CHAR_MAX 0x10ffff

/* Map TRE helpers to your wide-char stubs (ASCII semantics underneath) */
#define tre_isalnum  iswalnum
#define tre_isalpha  iswalpha
#define tre_isblank  iswblank
#define tre_iscntrl  iswcntrl
#define tre_isdigit  iswdigit
#define tre_isgraph  iswgraph
#define tre_islower  iswlower
#define tre_isprint  iswprint
#define tre_ispunct  iswpunct
#define tre_isspace  iswspace
#define tre_isupper  iswupper
#define tre_isxdigit iswxdigit

#define tre_tolower  towlower
#define tre_toupper  towupper
#define tre_strlen   wcslen

/* Use provided iswctype()/wctype() */
typedef wctype_t tre_ctype_t;
#define tre_isctype iswctype
#define tre_ctype   wctype

#define elementsof(x) (sizeof(x) / sizeof((x)[0]))

/* Safer ALIGN (uintptr_t) while keeping original intent */
#define ALIGN(ptr, type) \
  ((((uintptr_t)(ptr)) % sizeof(type)) \
   ? (sizeof(type) - (((uintptr_t)(ptr)) % sizeof(type))) \
   : 0)

#undef  MAX
#undef  MIN
#define MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

/* TNFA transition type. */
typedef struct tnfa_transition tre_tnfa_transition_t;

struct tnfa_transition {
	tre_cint_t code_min;
	tre_cint_t code_max;
	tre_tnfa_transition_t *state;
	int state_id;
	int *tags;
	int assertions;
	union {
		tre_ctype_t class;  /* keep field name 'class' */
		int backref;
	} u;
	tre_ctype_t *neg_classes;
};

/* Assertions. */
#define ASSERT_AT_BOL          1
#define ASSERT_AT_EOL          2
#define ASSERT_CHAR_CLASS      4
#define ASSERT_CHAR_CLASS_NEG  8
#define ASSERT_AT_BOW         16
#define ASSERT_AT_EOW         32
#define ASSERT_AT_WB          64
#define ASSERT_AT_WB_NEG     128
#define ASSERT_BACKREF       256
#define ASSERT_LAST          256

/* Tag directions. */
typedef enum {
	TRE_TAG_MINIMIZE = 0,
	TRE_TAG_MAXIMIZE = 1
} tre_tag_direction_t;

/* Submatch data. */
struct tre_submatch_data {
	int so_tag;
	int eo_tag;
	int *parents;
};
typedef struct tre_submatch_data tre_submatch_data_t;

/* TNFA definition. */
typedef struct tnfa tre_tnfa_t;

struct tnfa {
	tre_tnfa_transition_t *transitions;
	unsigned int num_transitions;
	tre_tnfa_transition_t *initial;
	tre_tnfa_transition_t *final;
	tre_submatch_data_t *submatch_data;
	char *firstpos_chars;
	int first_char;
	unsigned int num_submatches;
	tre_tag_direction_t *tag_directions;
	int *minimal_tags;
	int num_tags;
	int num_minimals;
	int end_tag;
	int num_states;
	int cflags;
	int have_backrefs;
	int have_approx;
};

/* --- tre-mem.h bridge --- */
#ifndef TRE_MEM_BLOCK_SIZE
#define TRE_MEM_BLOCK_SIZE 1024
#endif

typedef struct tre_list {
	void *data;
	struct tre_list *next;
} tre_list_t;

typedef struct tre_mem_struct {
	tre_list_t *blocks;
	tre_list_t *current;
	char *ptr;
	size_t n;
	int failed;
	void **provided;
} *tre_mem_t;

/* ‘hidden’ is a visibility macro in upstream; make it a no-op if absent. */
#ifndef hidden
#define hidden
#endif

#define tre_mem_new_impl   __tre_mem_new_impl
#define tre_mem_alloc_impl __tre_mem_alloc_impl
#define tre_mem_destroy    __tre_mem_destroy

hidden tre_mem_t tre_mem_new_impl(int provided, void *provided_block);
hidden void *tre_mem_alloc_impl(tre_mem_t mem, int provided, void *provided_block,
				int zero, size_t size);
hidden void tre_mem_destroy(tre_mem_t mem);

#define tre_mem_new()              tre_mem_new_impl(0, NULL)
#define tre_mem_alloc(mem, size)   tre_mem_alloc_impl(mem, 0, NULL, 0, (size))
#define tre_mem_calloc(mem, size)  tre_mem_alloc_impl(mem, 0, NULL, 1, (size))

/* Kernel mappings & build-time knobs you added earlier */
#ifdef assert
# undef assert
#endif
#define assert(x) ((void)0)

#define xmalloc   kmalloc
#define xcalloc   kcalloc
#define xfree     kfree
#define xrealloc  krealloc
#define calloc(n,s) kcalloc((n),(s))

#ifndef CHARCLASS_NAME_MAX
#define CHARCLASS_NAME_MAX 32
#endif

#ifndef RE_DUP_MAX
#define RE_DUP_MAX 255
#endif

#ifndef MB_LEN_MAX
#define MB_LEN_MAX 1
#endif

#endif /* TRE_INTERNAL_H_ */
