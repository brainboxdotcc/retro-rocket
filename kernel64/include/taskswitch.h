#ifndef __TASKSWITCH_H__
#define __TASKSWITCH_H__

#define PROC_RUNNING	0
#define PROC_IDLE	1
#define PROC_DELETE	2

struct process {
	/*Identification */
	u32			pid;	/*PROCESS ID */
	u32			ppid;	/*Parent PID */
	u32			uid;	/*User id - Future use */
	u32			gid;	/*Group id - Future use */
	u32			state;	/*Running state */
	u32			start_time;

	u32			waitpid;	/* PID we are waiting on compltion of */

	u8			cpu;

	char*			directory;
	char*			name;
	u32			size;
	unsigned char*		text;

	struct console*		cons;

	struct ubasic_ctx*	code;	/* uBASIC context */

	/*Process Management */
	struct process*		prev;	/* Prev Process */
	struct process*		next;	/* Next Process */
};

struct process* proc_load(const char* fullpath, struct console* cons);
struct process* proc_find(u32 pid);
struct process* proc_cur();
void proc_wait(struct process* proc, u32 otherpid);
void proc_run(struct process* proc);
int proc_ended(struct process* proc);
void proc_kill(struct process* proc);
void proc_show_list();
void proc_loop();
void proc_timer();
s64 proc_total();
const char* proc_name(s64 index);
u32 proc_id(s64 index);

#endif
