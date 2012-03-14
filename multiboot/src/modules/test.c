#include <module.h>

extern int modinit(KernelInfo* ki)
{
	int v = ki->findsym("test");
	return 0;
}

extern int modfini(KernelInfo* ki)
{
	return 0;
}
