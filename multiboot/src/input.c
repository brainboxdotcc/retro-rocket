#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/keyboard.h"
#include "../include/input.h"
#include "../include/timer.h"

unsigned int kinput(char* buffer, unsigned int maxlen, console* cons)
{
	int bufcnt = 0;
	char last = 0;
	blitconsole(cons);
	while (last != '\r')
	{
		last = kgetc(cons);
		switch (last)
		{
			case '\r':
				kprintf("\n");
			break;
			case 8:
				if (bufcnt != 0)
				{
					buffer--;
					bufcnt--;
					/* Advance text cursor back one space, if we are at x=0, move to x=79, y--.
					 * If we are at y=0, scroll up?
					 */
					cons->x--;
					kprintf(" ");
					cons->x--;
					setcursor(cons);
				}
				else
				{
					beep(1000);
				}
			break;
			default:
				if (bufcnt == maxlen)
				{
					beep(1000);
				}
				else
				{
					*buffer++ = last;
					bufcnt++;
					kprintf("%c", last);
				}
			break;
		}
	}

	/* Terminate string */
	*buffer = 0;

	return bufcnt;
}

