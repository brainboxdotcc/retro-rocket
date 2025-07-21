/**
 * @file input.h
 * @author Craig Edwards (craigedwards@brainbox.cc)
 * @copyright Copyright (c) 2012-2025
 */
#ifndef __INPUT_H__
#define __INPUT_H__

size_t kinput(size_t maxlen, console* cons);
void kfreeinput(console* cons);
char* kgetinput(console* cons);

#endif
