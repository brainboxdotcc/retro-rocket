#include <kernel.h>
#include <interrupts.h>
#include <kmalloc.h>
#include <string.h>
#include <elf.h>
#include <taskswitch.h>
#include <syscall.h>
#include <filesystem.h>

u32int ret_addr;
u32int ret_esp;

/* Syscall handler */
void syscall_handler(registers_t* regs);

void init_syscall(void)
{
	register_interrupt_handler(50, &syscall_handler);
}

void syscall_handler(registers_t* regs)
{
	kprintf("syscall 0x%04x\n", regs->eax);
	argv_t* argv;
	ret_addr = 0;
	ret_esp = 0;
	switch (regs->eax){
		/*------------ TERMINAL IO --------------- */
		case SYS_PUTCH:
			put(current_console, regs->ebx);
		break;
		case SYS_PUTS:
			putstring(current_console, (char*)regs->ebx);
		break;
		case SYS_CLEAR:
			clearscreen(current_console);
		break;
		case SYS_SETBGCLR:
			setbackground(current_console, (u8int)regs->ebx);
		break;
		case SYS_SETFGCLR:
			setforeground(current_console, (u8int)regs->ebx);
		break;

		case SYS_FOPEN: /*ebx == path, ecx = ret ptr */
			regs->eax = _open((char*)regs->ebx, regs->ecx);
		break;
		case SYS_FCLOSE: {	/*ebx == fd */
			_close(regs->ebx);
			break;
		}
		case SYS_FREAD: {	/*ebx == argument Vector pointer , ecx = ret ptr*/
			argv = (argv_t*)regs->ebx;
			if (argv->cnt != 3)
				return;
			regs->eax = _read(argv->vect[0],(unsigned char*)argv->vect[1],argv->vect[2]);
			break;
		}

		case SYS_MALLOC: {	/*ebx == size */
			regs->eax = (u32int)malloc(regs->ebx);
			break;
		}
		case SYS_FREE: {	/*ebx == address */
			free((void*)regs->ebx);
			break;
		}

		default:
			kprintf("Unknown syscall 0x%04x!\n", regs->eax);
		break;
	}

	if (ret_addr)
	{
		/* Return address changed, probably by fork or exec */
		kprintf("ret_addr changed, eip was %d now %d\n", regs->eip, ret_addr);
		regs->eip = ret_addr;
		if (ret_esp)
		{
			regs->useresp = ret_esp;
		}
	}
}

