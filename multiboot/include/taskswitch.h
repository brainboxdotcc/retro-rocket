#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#include "kernel.h"
#include "paging.h"
#include "video.h"
#include "interrupts.h"

#define PROC_RUNNING	0
#define PROC_IDLE	1
#define PROC_DELETE	2

typedef struct {
	/*Identification */
	u32int			pid;	/*PROCESS ID */
	u32int			ppid;	/*Parent PID */
	u32int			uid;	/*User id - Future use */
	u32int			gid;	/*Group id - Future use */
	u32int			state;	/*Running state */

	/*Process Switching */
	registers_t		regs;	/*Current State (gia return) */
	console*		tty;	/*Terminal - 0 == ignore IO */
	page_directory_t*	dir;	/*to Address Space tou proc */
	u32int			kstack;	/*Kernel Stack */

	/*Process Management */
	void*			prev;	/*Prev Process */
	void*			next;	/*Next Process */
}process_t;

/*Prototypes - KERNEL mode ONLY */
void	init_process_manager(void);	/*Initialisation kai dimiourgia 1ou Process */
void	proc_switch(registers_t* regs);	/*O Scheduler mas - Klisi se afton == Process change */
void	proc_chstack(u32int address, u32int size);	/*Metafora stack ektos kernel space */
void	proc_set_semaphore(void);	/*Disable Multitasking */
void	proc_clear_semaphore(void);	/*Enable Multitasking */
u32int	proc_change_tty(u32int tty);	/*Change Process Terminal */
void	start_initial_task(void);	/*Start init() se USER MODE */

/*Process Manager USER-MODE Interface */
void	set_state(u32int state);
u32int	fork(registers_t* regs);
u32int	getpid(void);
u32int	getppid(void);
void	exit(void);
void	wait(u32int pid);
u32int get_init_size();

#endif
