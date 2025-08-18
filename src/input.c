#include <kernel.h>

extern spinlock_t console_spinlock;
extern spinlock_t debug_console_spinlock;
static bool stopped = false;
uint8_t last = 0;		/**< Last character written. */
char* internalbuffer = NULL;	/**< Optional backing buffer for console text. */
char* buffer = NULL;		/**< External buffer, if used. */
size_t bufcnt = 0;		/**< Buffer size or counter. */

size_t kinput(size_t maxlen) {
	last = kgetc();

	if (stopped && buffer != NULL) {
		return 1;
	}

	if (last == 255) {
		__asm__ volatile("hlt");
		return 0;
	}

	if (buffer == NULL) {
		stopped = false;
		internalbuffer = kmalloc(maxlen + 1);
		if (!internalbuffer) {
			return 0;
		}
		buffer = internalbuffer;
		bufcnt = 0;
	}

	uint64_t flags;
	lock_spinlock_irq(&console_spinlock, &flags);
	lock_spinlock(&debug_console_spinlock);
	switch (last) {
		case KEY_UP:
		case KEY_DOWN:
		case KEY_LEFT:
		case KEY_RIGHT:
		break;
		case '\r':
			put('\n');
		break;
		case 8:
			if (bufcnt != 0) {
				buffer--;
				bufcnt--;
				/* Advance text cursor back one space, if we are at x=0, move to x=79, y--.
				 * If we are at y=0, scroll up?
				 */
				put(8);
				put(' ');
				put(8);
			} else {
				beep(1000);
			}
		break;
		default:
			if (bufcnt == maxlen) {
				beep(1000);
			} else {
				*(buffer++) = last;
				bufcnt++;
				put(last);
			}
		break;
	}
	unlock_spinlock(&debug_console_spinlock);
	unlock_spinlock_irq(&console_spinlock, flags);
	/* Terminate string */
	*buffer = 0;

	if (last == '\r') {
		stopped = true;
	}

	return last == '\r';
}

void kfreeinput() {
	kfree_null(&internalbuffer);
	buffer = NULL;
	bufcnt = 0;
	stopped = false;
}

char* kgetinput() {
	return internalbuffer;
}
