int _start()
{
	char x[1024];
	char n[1024];
	int q = 0;
	const char* t = n;
	for (; *t; ++t)
	{
		x[++q] = *t;
	}
	return 1;
}
