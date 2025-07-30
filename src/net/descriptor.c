#include <kernel.h>

static tcp_conn_t* fd[FD_MAX] = { 0 };

int tcp_allocate_fd(tcp_conn_t* conn)
{
	for (size_t x = 0; x < FD_MAX; ++x) {
		if (fd[x] == NULL) {
			fd[x] = conn;
			return x;
		}
	}
	return -1;
}

void tcp_free_fd(int x)
{
	if (x >= 0 && x < FD_MAX) {
		fd[x] = NULL;
	}
}

tcp_conn_t* tcp_find_by_fd(int x)
{
	if (x >= 0 && x < FD_MAX) {
		return fd[x];
	}
	return NULL;
}

