#ifndef __KEYBOARD_H__
#define __KEYBOARD_H__

#include "kernel.h"
#include "video.h"

void init_basic_keyboard();
char kgetc(console* cons);

#endif
