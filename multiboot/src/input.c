#include <string.h>
#include <kmalloc.h>
#include <keyboard.h>
#include <input.h>
#include <timer.h>

/* Return values:
 * 0: Not finished entering a line
 * 1: Line complete
 */

unsigned int kinput(unsigned int maxlen, console* cons)
{
	blitconsole(cons);
	
	cons->last = kgetc(cons);
	
	if (cons->last == 255)
	{
		asm volatile("hlt");
		return 0;
	}

	if (cons->buffer == NULL)
	{
		cons->internalbuffer = (char*)kmalloc(maxlen + 1);
		cons->buffer = cons->internalbuffer;
		cons->bufcnt = 0;
		//kprintf("new buf");
	}

	switch (cons->last)
	{
		case '\r':
			kprintf("\n");
		break;
		case 8:
			if (cons->bufcnt != 0)
			{
				cons->buffer--;
				cons->bufcnt--;
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
			if (cons->bufcnt == maxlen)
			{
				beep(1000);
			}
			else
			{
				*(cons->buffer++) = cons->last;
				cons->bufcnt++;
				kprintf("%c", cons->last);
			}
		break;
	}
	/* Terminate string */
	*cons->buffer = 0;

	return cons->last == '\r';
}

void kfreeinput(console* cons)
{
	kfree(cons->internalbuffer);
	cons->internalbuffer = cons->buffer = NULL;
}

char* kgetinput(console* cons)
{
	return cons->internalbuffer;
}
