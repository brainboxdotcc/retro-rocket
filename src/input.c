#include <kernel.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;

/* Return values:
 * 0: Not finished entering a line
 * 1: Line complete
 */

size_t kinput(size_t maxlen, console* cons)
{
	cons->last = kgetc(cons);
	
	if (cons->last == 255) {
		__asm__ volatile("hlt");
		return 0;
	}

	if (cons->buffer == NULL) {
		cons->internalbuffer = kmalloc(maxlen + 1);
		if (!cons->internalbuffer) {
			return 0;
		}
		cons->buffer = cons->internalbuffer;
		cons->bufcnt = 0;
	}

	uint64_t flags;
	lock_spinlock_irq(&console_spinlock, &flags);
	lock_spinlock(&debug_console_spinlock);
	switch (cons->last) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		break;
		case '\r':
			put(cons, '\n');
		break;
		case 8:
			if (cons->bufcnt != 0) {
				cons->buffer--;
				cons->bufcnt--;
				/* Advance text cursor back one space, if we are at x=0, move to x=79, y--.
				 * If we are at y=0, scroll up?
				 */
				put(cons, 8);
				put(cons, ' ');
				put(cons, 8);
			} else {
				beep(1000);
			}
		break;
		default:
			if (cons->bufcnt == maxlen) {
				beep(1000);
			} else {
				*(cons->buffer++) = cons->last;
				cons->bufcnt++;
				put(cons, cons->last);
			}
		break;
	}
	unlock_spinlock(&debug_console_spinlock);
	unlock_spinlock_irq(&console_spinlock, flags);
	/* Terminate string */
	*cons->buffer = 0;

	return cons->last == '\r';
}

void kfreeinput(console* cons)
{
	kfree_null(&cons->internalbuffer);
	cons->internalbuffer = cons->buffer = NULL;
	cons->bufcnt = 0;
}

char* kgetinput(console* cons)
{
	return cons->internalbuffer;
}
