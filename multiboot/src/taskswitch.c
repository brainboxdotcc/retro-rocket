#include "../include/kernel.h"
#include "../include/taskswitch.h"
#include "../include/paging.h"
#include "../include/filesystem.h"
#include "../include/interrupts.h"
#include "../include/syscall.h"
#include "../include/elf.h"
#include "../include/printf.h"
#include "../include/kmalloc.h"
#include "../include/memcpy.h"

process_t* proc_current = 0;
process_t* proc_list;

u32int nextid = 1;
u32int semaphore = 0;
u32int deathflag = 0;

extern page_directory_t *current_directory;	/*Gia to change stack, apo paging.c */
extern u32int initial_esp;			/*Episis gia stack change, apo kernel.c */
extern console* current_console;		/*Xreiazetai gia to initialise */
extern u32int ret_addr;				/*Ret address meta apo syscall */
extern u32int ret_esp;				/*Ret esp meta apo syscall */
extern tss_entry_t tss_entry;			/* Task state segment */

/*Voithitikes routines */
//static u32int read_eip();	/*Epistrefei to Instruction pointer ap to simeio pou klithike */
static process_t* next_proc();
static u32int checkpid(u32int pid);
static void proc_clear();	/*Psaxnei teliomena processes kai ta kanei kill */
static process_t* proc_kill(process_t*);

void set_kernel_stack(u32int stack){
	tss_entry.esp0 = stack;
}

void init()
{
	u32int pid = 0;

	asm volatile("int $50" : : "a"(SYS_FORK));
	asm volatile("mov %%eax, %0" : "=a"(pid));
	if (!pid)
	{
		asm volatile("int $50" : : "a"(SYS_SETMTX));
		asm volatile("int $50" : : "a"(SYS_EXEC), "b"("/programs/consh"));
		asm volatile("int $50" : : "a"(SYS_CLRMTX));
	}
	else
	{
		while(1);       // System idle process
	}
}

/* Do NOT move this from immediately after init() */
static void byte_after_init()
{
}

u32int get_init_size()
{
	return (u32int)((u32int)&byte_after_init - (u32int)&init);
}

void init_process_manager(void){
	process_t* fst_proc;

	deathflag = 0;
	fst_proc = (process_t*)kmalloc(sizeof(process_t));	/*Alloc xoro gia to process */
	_memset((char*)fst_proc, 0 , sizeof(process_t));		/*Nullify */

	asm volatile("cli");					/*Den theloume na ginei apopeira switch */
	proc_chstack(USTACK - 0x4, USTACK_SIZE);		/*Metaferoume stack se User Space */

	/*DEN xreiazete na ftiaksoume STATE kathws o scheduler sozei
	  to current state sto Current process kai kanei load to epomeno.
	  Sto simio loipon pou exoume 1 process mono, to State tha graftei
	  ap ton scheduler kai tha ksanaginei load. apla :) */

	/*Initialize to 1o process, den xreiazete eksigisi */
	fst_proc->state = PROC_RUNNING;
	fst_proc->pid = nextid++;
	fst_proc->ppid = 0;
	fst_proc->tty = current_console;
	fst_proc->dir = current_directory;
	fst_proc->kstack = (u32int)kmalloc_ext(0x1000, 1, 0);	/*Alloc to kernel stack tou */
	_memset((char*)fst_proc->kstack, 0 , 0x1000);	/*Nullify */
	fst_proc->kstack += 0x1000-4;
	set_kernel_stack(proc_current->kstack);		/*Orizoume to esp0 tou TSS oste na ginei load otan kanoume
							  jump se user mode. Pali tha mporouse na ginei init apo ton
							  scheduler, alla logo krisimotitas, pronooume */
	fst_proc->next = 0;	/*Monadiko stoixeio stin lista */
	fst_proc->prev = 0;

	proc_current = fst_proc;	/*Arxikopoioume kai ta global */
	proc_list = fst_proc;

	proc_clear_semaphore();		/*Enable multitasking */

	asm volatile("sti");		/*Teliosame */
}

void proc_set_semaphore(void){	/*Enable multitasking */
	while (!semaphore)
		semaphore = 1;
}

void proc_clear_semaphore(void){/*Disable multitasking */
	semaphore = 0;
}

u32int getpid(void){
	return proc_current->pid;
}

u32int getppid(void){		/*Get process ID */
	return proc_current->ppid;
}

u32int proc_change_tty(u32int tty){	/* XXX: STUB */
	console* tmp;
	tmp = current_console;
	if (tmp)
		proc_current->tty = tmp;
	return (u32int)tmp;
}

void	set_state(u32int state){
	proc_current->state = state;
}

void	exit(void){
	process_t* parent;
	for (parent = proc_list; parent ; parent = parent->next){
		if (parent->pid == proc_current->ppid){ 
			parent->state = PROC_RUNNING;
			break;
		}
	}
	proc_current->state = PROC_DELETE;
}

void	wait(u32int pid){
	process_t* chld = (void*)checkpid(pid);
	while (chld){
		chld = (void*)checkpid(pid);
		if (!chld) break;
		if (chld->state == PROC_DELETE) break;
		proc_current->state = PROC_IDLE;
	}
	proc_current->state = PROC_RUNNING;
}

void proc_switch(registers_t* regs)
{
	/*Contex Switch routina */
/*CRITICAL NOTE: O scheduler exei sxediastei gia xrisi Kernel stack, mesa se Kernel space,
		 kathws kanei switch page directory. Stin periptosi pou den exoume mpei akoma
		 se User mode (opote kai xrisi 2 stack), SE KAMIA PERIPTOSI den prepei na uparxoun
		 2 diergasies, alla mono 1. Meta tin metavasi se User mode, eimaste ok */
	if (proc_current == 0)	/*Unitialised Process Manager */
		return;
 	if (semaphore == 1)	/*Disabled multitasking */
		return;

	asm volatile("cli");	/*Den theloume interrupts oso kanoume switch */
	/*Sozoume current state se current process */
	memcpy(&proc_current->regs, regs, sizeof(registers_t));

	/*Psaxnoume to epomeno proc */
	proc_current = next_proc();

	/*Load to state tou neou process */
	memcpy(regs, &proc_current->regs, sizeof(registers_t));

	/*Kai kanoume switch */
	switch_page_directory(proc_current->dir);
	set_kernel_stack(proc_current->kstack);

	/*H iret tha kanei return sto neo state pleon, sto neo Address Space */
	if (deathflag)
		proc_clear();
}

u32int fork(registers_t* regs)
{	/*Copy STATE+Address space se neo process */
/*To child tha arxisei na ekteleite apo ekei pou tha kanei return afti i fork. To 
  sygkekrimeno body tha ektelestei MONO sto parent process */
	process_t *parent, *child, *tmp;
	u32int ret = 0;

	/*Arxikopoioume voithitikes vars */
	parent = proc_current;
	child = (process_t*)kmalloc(sizeof(process_t));
	_memset((char*)child, 0, sizeof(process_t));

	/*Arxikopoioume to child process */
	child->state = PROC_RUNNING;
	child->pid = nextid++;
	child->ppid = parent->pid;
	child->tty = parent->tty;
	child->uid = parent->uid;
	child->gid = parent->gid;
	child->kstack = (u32int)kmalloc_ext(0x1000,1,0);		/*diko tou kernel stack */
	_memset((char*)child->kstack, 0 , 0x1000);		/*Nullify */
	child->kstack += 0x1000-4;
	memcpy(&child->regs, regs, sizeof(registers_t));/*State copy */
	child->regs.eax = 0;					/*Return == 0 sto iret */

	/*Vriskoume to telefteo proc stin lista */
	tmp = proc_list;
	while (tmp->next)
		tmp = tmp->next;

	asm volatile("cli");	/*Gia na min kanei switch edw */

	/*Prosthetoume to child stin lista */
	tmp->next = child;
	child->prev = tmp;
	child->next = 0;

	/*Clone Adress Space */
	child->dir = clone_directory(parent->dir);

	/*To return value */
	ret = child->pid;
	asm volatile("sti");
	return ret;
}

void exec(char* path)
{
	/* Replace core image with new binary and execute it */
	load_elf(path);
}

void proc_chstack(u32int address, u32int size){	/*Move Current Stack */
	u32int i,*p;
	u32int old_sp, old_bp;	/*Ta esp ebp pou isxioun */
	u32int new_sp, new_bp;	/*Ta esp ebp pou tha orisoume, tha xreiastoun gia
				  to re-positioning twn pointers mesa sto stack */

	i = address - size;	/*Upologizoume to base */
	i &= 0xFFFFF000;	/*Se page Bound */

	sign_sect(i, (address & 0xFFFFF000) + 0x1000, 1, 1, current_directory);	/*User mode */

	asm volatile("mov %%esp, %0" : "=r" (old_sp));
	asm volatile("mov %%ebp, %0" : "=r" (old_bp));
	
	/*Ypologizoume tin diafora metaksi arxikou stack me neou
	  Na simiosoume oti exoume apothikefsei ton arxiko stack pointer
	  sto initial_esp sto kernel.c, efoson theloume na antigrapsoume olo 
	  to stack apo to entry point tou purina kai meta */
	i = address - initial_esp;
	
	new_sp = old_sp + i;
	new_bp = old_bp + i;

	/*Menei loipon na kanoume copy ta Stack data, kai fix ta pointers pou vriskonte 
	mesa sto palio stack range, pithana push ebp oste na teriazoun sto new */
	memcpy((char*)new_sp, (char*)old_sp, initial_esp - old_sp);

	/*Psaxnoume tora ta data kai fix ta addresses mesa sto stack */
	for (p = (void*)(address - size); address >= (u32int)p ; p++){
		if (*p >= old_sp && *p <= initial_esp)	/*Address sto palio stack range */
			*p += i;	/*fix += tin diafora twn old stack me new stack */
	}
	/*Telos fortonoume ta 2 registers pou sxetizonte me to stack stis nees times */
	asm volatile("mov %0, %%esp" : : "r" (new_sp));
	asm volatile("mov %0, %%ebp" : : "r" (new_bp));
}

void start_initial_task(void){
	u32int size, place;
// 	u32int eip;
	u32int eflags;

	asm volatile ("sti;");	
	/*Sozoume flags me IF = 1 ( mporousame |= 0x200 ) */
	asm volatile ("	pushf;\
			pop %%eax;\
			movl %%eax, %0; " : "=r"(eflags)
	);
	asm volatile ("cli;");	/*disable interrupts */
// 	eip = read_eip();
	size = get_init_size();
	place = 0x40000000;

	/*Sign ta pages se User mode RW */
	sign_sect(place,place + size, 1, 1,current_directory);
	memcpy((char*)place, (char*)&init, size);		/*Copy ton kodika tis init */
	/*Kai ftiaxnoume ena fake IRET gia na girisoume se User Mode */
	/*orizoume data segment selectors */
	asm volatile("\
		mov $0x23, %ax; \
		mov %ax, %ds; \
		mov %ax, %es; \
		mov %ax, %fs; \
		mov %ax, %gs; \
		pushl $0x23; \
		pushl $0x7FFFFFFC; \
	");	
	/*to pushl $0x7FFFF004; eiani apla ena push mesa sto stack gia esp, opoudipote, kalo omos
	  tha itan na ginei stin arxi tou stack, Na simiothei oti gia na glitosoume transaction valame
	  fixed value,NOTICE den einai compatible me USTACK kai USTACK_SIZE pou exoume orisei sta header */
	asm volatile("pushl %%eax;" : : "a"(eflags));
	asm volatile("\
		pushl $0x1B; \
		pushl $0x40000000; \
		iret; \
	");
/*Vriskomaste se KERNEL SPACE, i init DEN mporei na kanei return edw apo User space.
  Par ola afta, an thelisoume gia opoiodipote logo na sosoume to "call" apo edw,
  kanoume uncomment ta comented lines */
}

static process_t* next_proc(void)
{
	process_t* ret;

	ret = proc_current->next;
	while (ret)
	{
		if (ret->state == PROC_DELETE)
			deathflag = 1;
		if (ret->state == PROC_RUNNING)
			return ret;
		ret = ret->next;
	}
	/*Ap tin arxi */
	ret = proc_list;
	while (ret != proc_current)
	{
		if (ret->state == PROC_DELETE)
			deathflag = 1;
		if (ret->state == PROC_RUNNING)
			return ret;
		ret = ret->next;
	}
	/*Den vrethike allo running proc */
	return proc_current;
}

static void proc_clear()
{
	process_t* tmp;
	tmp = proc_list->next;	/*Den mporei to init na einai dead */
	while (tmp)
	{
		if (tmp->state == PROC_DELETE)
			tmp = proc_kill(tmp);
		tmp = tmp->next;
	}
	deathflag = 0;
}

static process_t* proc_kill(process_t* p){
	process_t *prev, *next;
	prev = p->prev;
	next = p->next;
	prev->next = next;
	if (next) next->prev = prev;

	kfree((void*)(p->kstack - 0x1000 +4));
	kill_directory(p->dir);
	kfree(p);

	return prev;
}

static u32int checkpid(u32int pid){
	process_t* ret = 0;
	for (ret = proc_list; ret ; ret = ret->next)
		if (ret->pid == pid) break;
	return (u32int)ret;
}

//static u32int read_eip()
//{
//	asm volatile("popl %ebx");	/*pop Base pointer pou egine aftomata push logo __cdecl conv*/
//	asm volatile("popl %eax");	/*pop Instruction pointer gia return -- To opoio kai theloume */
//	asm volatile("pushl %eax");	/*Ksana push gia na epanaferoume tin stiva opos itan */
//	asm volatile("pushl %ebx");
//}

