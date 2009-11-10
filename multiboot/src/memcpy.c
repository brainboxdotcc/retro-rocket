/********************************************************************
 ** File:	 memcpy.c
 **
 ** Copyright (C) 2005 Daniel Vik
 ** 
 ** This software is provided 'as-is', without any express or implied
 ** warranty. In no event will the authors be held liable for any
 ** damages arising from the use of this software.
 ** Permission is granted to anyone to use this software for any
 ** purpose, including commercial applications, and to alter it and
 ** redistribute it freely, subject to the following restrictions:
 ** 
 ** 1. The origin of this software must not be misrepresented; you
 **	must not claim that you wrote the original software. If you
 **	use this software in a product, an acknowledgment in the
 **	use this software in a product, an acknowledgment in the
 **	product documentation would be appreciated but is not
 **	required.
 ** 
 ** 2. Altered source versions must be plainly marked as such, and
 **	must not be misrepresented as being the original software.
 ** 
 ** 3. This notice may not be removed or altered from any source
 **	distribution.
 ** 
 ** 
 ** Description: Implementation of the standard library function memcpy.
 ** This implementation of memcpy() is ANSI-C89 compatible.
 ** The following configuration options can be set:
 **
 **   LITTLE_ENDIAN     - Uses processor with little endian
 **                       addressing. Default is big endian.
 **
 **   PRE_INC_PTRS      - Use pre increment of pointers.
 **                       Default is post increment of
 **                       pointers.
 **
 **   INDEXED_COPY      - Copying data using array indexing.
 **                       Using this option, disables the
 **                       PRE_INC_PTRS option.
 **
 **
 ** Best Settings:
 **
 ** Intel x86:  LITTLE_ENDIAN and INDEXED_COPY
 **
 *******************************************************************/

#include "../include/kernel.h"
#include "../include/memcpy.h"

/********************************************************************
 ** Configuration definitions.
 *******************************************************************/

#define LITTLE_ENDIAN
#define INDEXED_COPY


/********************************************************************
 ** Remove definitions when INDEXED_COPY is defined.
 *******************************************************************/

#if defined (INDEXED_COPY)
#if defined (PRE_INC_PTRS)
#undef PRE_INC_PTRS
#endif /*PRE_INC_PTRS*/
#endif /*INDEXED_COPY*/



/********************************************************************
 ** Definitions for pre and post increment of pointers.
 *******************************************************************/

#if defined (PRE_INC_PTRS)

#define INC_VAL(x) *++(x)
#define START_VAL(x) (x)--
#define CAST_32_TO_8(p, o)	   (u8int *)((u32int)p + o + 4)
#define WHILE_DEST_BREAK		 3
#define PRE_LOOP_ADJUST		- 3
#define PRE_SWITCH_ADJUST	  + 1

#else /*PRE_INC_PTRS*/

#define START_VAL(x)
#define INC_VAL(x) *(x)++
#define CAST_32_TO_8(p, o)	   (u8int *)((u32int)p + o)
#define WHILE_DEST_BREAK		 0
#define PRE_LOOP_ADJUST
#define PRE_SWITCH_ADJUST

#endif /*PRE_INC_PTRS*/



/********************************************************************
 ** Definitions for endians
 *******************************************************************/

#if defined (LITTLE_ENDIAN)

#define SHL >>
#define SHR <<

#else /*LITTLE_ENDIAN*/

#define SHL <<
#define SHR >>

#endif /*LITTLE_ENDIAN*/



/********************************************************************
 ** Macros for copying 32 bit words of  different alignment.
 ** Uses incremening pointers.
 *******************************************************************/

#define CP32_INCR() {					   \
	INC_VAL(dst32) = INC_VAL(src32);		\
}

#define CP32_INCR_SH(shl, shr) {			\
	dstWord   = srcWord SHL shl;			\
	srcWord   = INC_VAL(src32);			 \
	dstWord  |= srcWord SHR shr;			\
	INC_VAL(dst32) = dstWord;			   \
}



/********************************************************************
 ** Macros for copying 32 bit words of  different alignment.
 ** Uses array indexes.
 *******************************************************************/

#define CP32_INDEX(idx) {				   \
	dst32[idx] = src32[idx];				\
}

#define CP32_INDEX_SH(x, shl, shr) {		\
	dstWord   = srcWord SHL shl;			\
	srcWord   = src32[x];				   \
	dstWord  |= srcWord SHR shr;			\
	dst32[x] = dstWord;					 \
}



/********************************************************************
 ** Macros for copying 32 bit words of different alignment.
 ** Uses incremening pointers or array indexes depending on
 ** configuration.
 *******************************************************************/

#if defined (INDEXED_COPY)

#define CP32(idx)			   CP32_INDEX(idx)
#define CP32_SH(idx, shl, shr)  CP32_INDEX_SH(idx, shl, shr)

#define INC_INDEX(p, o)		 ((p) += (o))

#else /*INDEXED_COPY*/

#define CP32(idx)			   CP32_INCR()
#define CP32_SH(idx, shl, shr)  CP32_INCR_SH(shl, shr)

#define INC_INDEX(p, o)

#endif /*INDEXED_COPY*/



/********************************************************************
 **
 ** void *memcpy(void *dest, const void *src, u32int count)
 **
 ** Args:	 dest	- pointer to destination buffer
 **		   src	 - pointer to source buffer
 **		   count   - number of bytes to copy
 **
 ** Return:   A pointer to destination buffer
 **
 ** Purpose:  Copies count bytes from src to dest. No overlap check
 **		   is performed.
 **
 *******************************************************************/

void *memcpy(void *dest, const void *src, u32int count)
{
	u8int *dst8 = (u8int *)dest;
	u8int *src8 = (u8int *)src;

	if (count < 8)
	{
		if (count >= 4 && ((((u32int)src8 | (u32int)dst8)) & 3) == 0) {
			*((u32int *)dst8) = *((u32int *)src8);
			dst8  += 4;
			src8  += 4;
			count -= 4;
		}

		START_VAL(dst8);
		START_VAL(src8);

		while (count--)
	{
			INC_VAL(dst8) = INC_VAL(src8);
		}

		return dest;
	}

	START_VAL(dst8);
	START_VAL(src8);

	while (((u32int)dst8 & 3L) != WHILE_DEST_BREAK) {
		INC_VAL(dst8) = INC_VAL(src8);
		count--;
	}

	switch ((((u32int)src8) PRE_SWITCH_ADJUST) & 3L) {
	default:
		{
			u32int *dst32 = (u32int *)(((u32int)dst8) PRE_LOOP_ADJUST);
			u32int *src32 = (u32int *)(((u32int)src8) PRE_LOOP_ADJUST);
			u32int length = count / 4;

			while (length & 7) {
				CP32_INCR();
				length--;
			}

			length /= 8;

			while (length--) {
				CP32(0);
				CP32(1);
				CP32(2);
				CP32(3);
				CP32(4);
				CP32(5);
				CP32(6);
				CP32(7);

				INC_INDEX(dst32, 8);
				INC_INDEX(src32, 8);
			}

			src8 = CAST_32_TO_8(src32, 0);
			dst8 = CAST_32_TO_8(dst32, 0);

			if (count & 2) {
				*dst8++ = *src8++;
				*dst8++ = *src8++;
			}

			if (count & 1) {
				*dst8 = *src8;
			}

			return dest;
		}

	case 1:
		{
			u32int *dst32  = (u32int *)((((u32int)dst8) PRE_LOOP_ADJUST) & ~3L);
			u32int *src32  = (u32int *)((((u32int)src8) PRE_LOOP_ADJUST) & ~3L);
			u32int length  = count / 4;
			u32int srcWord = INC_VAL(src32);
			u32int dstWord;

			while (length & 7) {
				CP32_INCR_SH(8, 24);
				length--;
			}

			length /= 8;

			while (length--) {
				CP32_SH(0, 8, 24);
				CP32_SH(1, 8, 24);
				CP32_SH(2, 8, 24);
				CP32_SH(3, 8, 24);
				CP32_SH(4, 8, 24);
				CP32_SH(5, 8, 24);
				CP32_SH(6, 8, 24);
				CP32_SH(7, 8, 24);

				INC_INDEX(dst32, 8);
				INC_INDEX(src32, 8);
			}

			src8 = CAST_32_TO_8(src32, -3);
			dst8 = CAST_32_TO_8(dst32, 0);

			if (count & 2) {
				*dst8++ = *src8++;
				*dst8++ = *src8++;
			}

			if (count & 1) {
				*dst8 = *src8;
			}

			return dest;
		}

	case 2:
		{
			u32int *dst32  = (u32int *)((((u32int)dst8) PRE_LOOP_ADJUST) & ~3L);
			u32int *src32  = (u32int *)((((u32int)src8) PRE_LOOP_ADJUST) & ~3L);
			u32int length  = count / 4;
			u32int srcWord = INC_VAL(src32);
			u32int dstWord;

			while (length & 7) {
				CP32_INCR_SH(16, 16);
				length--;
			}

			length /= 8;

			while (length--) {
				CP32_SH(0, 16, 16);
				CP32_SH(1, 16, 16);
				CP32_SH(2, 16, 16);
				CP32_SH(3, 16, 16);
				CP32_SH(4, 16, 16);
				CP32_SH(5, 16, 16);
				CP32_SH(6, 16, 16);
				CP32_SH(7, 16, 16);

				INC_INDEX(dst32, 8);
				INC_INDEX(src32, 8);
			}

			src8 = CAST_32_TO_8(src32, -2);
			dst8 = CAST_32_TO_8(dst32, 0);

			if (count & 2) {
				*dst8++ = *src8++;
				*dst8++ = *src8++;
			}

			if (count & 1) {
				*dst8 = *src8;
			}

			return dest;
		}

	case 3:
		{
			u32int *dst32  = (u32int *)((((u32int)dst8) PRE_LOOP_ADJUST) & ~3L);
			u32int *src32  = (u32int *)((((u32int)src8) PRE_LOOP_ADJUST) & ~3L);
			u32int length  = count / 4;
			u32int srcWord = INC_VAL(src32);
			u32int dstWord;

			while (length & 7) {
				CP32_INCR_SH(24, 8);
				length--;
			}

			length /= 8;

			while (length--)
			{
				CP32_SH(0, 24, 8);
				CP32_SH(1, 24, 8);
				CP32_SH(2, 24, 8);
				CP32_SH(3, 24, 8);
				CP32_SH(4, 24, 8);
				CP32_SH(5, 24, 8);
				CP32_SH(6, 24, 8);
				CP32_SH(7, 24, 8);

				INC_INDEX(dst32, 8);
				INC_INDEX(src32, 8);
			}

			src8 = CAST_32_TO_8(src32, -1);
			dst8 = CAST_32_TO_8(dst32, 0);

			if (count & 2)
			{
				*dst8++ = *src8++;
				*dst8++ = *src8++;
			}

			if (count & 1)
			{
				*dst8 = *src8;
			}

			return dest;
		}
	}
}
