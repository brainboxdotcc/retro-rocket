#include <kernel.h>

int64_t basic_getproccount(struct basic_ctx* ctx)
{
	return proc_total();
}

int64_t basic_get_free_mem(struct basic_ctx* ctx)
{
	return get_free_memory();
}

int64_t basic_get_used_mem(struct basic_ctx* ctx)
{
	return get_used_memory();
}

int64_t basic_get_total_mem(struct basic_ctx* ctx)
{
	return get_total_memory();
}

int64_t basic_getprocid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCID", 0);
	return proc_id(intval);
}

char* basic_getprocname(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCNAME$","");
	process_t* process = proc_find(proc_id(intval));
	return process && process->name ? gc_strdup(process->name) : "";
}

int64_t basic_getprocparent(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCPARENT", 0);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->ppid : 0;
}

int64_t basic_getproccpuid(struct basic_ctx* ctx)
{
	PARAMS_START;
	PARAMS_GET_ITEM(BIP_INT);
	PARAMS_END("GETPROCCPUID", 0);
	process_t* process = proc_find(proc_id(intval));
	return process ? process->cpu : 0;
}

