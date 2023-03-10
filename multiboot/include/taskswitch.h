#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#include "kernel.h"
#include "paging.h"
#include "video.h"
#include "ubasic.h"

#define PROC_RUNNING	0
#define PROC_IDLE	1
#define PROC_DELETE	2

struct process {
	/*Identification */
	u32int			pid;	/*PROCESS ID */
	u32int			ppid;	/*Parent PID */
	u32int			uid;	/*User id - Future use */
	u32int			gid;	/*Group id - Future use */
	u32int			state;	/*Running state */
	u32int			start_time;

	u32int			waitpid;	/* PID we are waiting on compltion of */

	char*			directory;
	char*			name;
	u32int			size;
	unsigned char*		text;

	struct console*		cons;

	struct ubasic_ctx*	code;	/* uBASIC context */

	/*Process Management */
	struct process*		prev;	/* Prev Process */
	struct process*		next;	/* Next Process */
};

struct process* proc_load(const char* fullpath, struct console* cons);
struct process* proc_find(u32int pid);
struct process* proc_cur();
void proc_wait(struct process* proc, u32int otherpid);
void proc_run(struct process* proc);
int proc_ended(struct process* proc);
void proc_kill(struct process* proc);
void proc_show_list();
void proc_loop();
void proc_timer();

#endif
