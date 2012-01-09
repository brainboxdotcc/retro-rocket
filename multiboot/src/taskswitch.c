#include "../include/kernel.h"
#include "../include/taskswitch.h"
#include "../include/paging.h"
#include "../include/filesystem.h"
#include "../include/interrupts.h"
#include "../include/syscall.h"
#include "../include/string.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"
#include "../include/kprintf.h"

struct process* proc_current = NULL;
struct process* proc_list = NULL;

u32int nextid = 1;

struct process* proc_load(const char* fullpath, struct console* cons)
{
	FS_DirectoryEntry* fsi = fs_get_file_info(fullpath);
	if (fsi != NULL)
	{
		unsigned char* programtext = (unsigned char*)kmalloc(fsi->size + 1);
		*(programtext + fsi->size) = 0;
		if (fs_read_file(fsi, 0, fsi->size, programtext))
		{
			//kprintf("program len = %d size = %d\n", strlen(programtext), fsi->size);
			struct process* newproc = (struct process*)kmalloc(sizeof(struct process));
			newproc->code = ubasic_init((const char*)programtext, (console*)cons);
			newproc->name = strdup(fsi->filename);
			newproc->pid = nextid++;
			newproc->size = fsi->size;
			newproc->cons = cons;
			newproc->text = programtext;
			return newproc;
		}
		else
		{
			kfree(programtext);
		}

		return NULL;
	}

	return NULL;
}

void proc_run(struct process* proc)
{
	ubasic_run(proc->code);
}

void proc_kill(struct process* proc)
{
	//kprintf("prog: '%s'\n", proc->code->program_ptr);
	kprintf("proc_kill\n");
	ubasic_destroy(proc->code);
	kfree(proc->name);
	kfree(proc->text);
}

int proc_ended(struct process* proc)
{
	int r = ubasic_finished(proc->code);
	//kprintf("r=%d\n", r);
	return r;
}

