#include <kernel.h>

inline queue_t* queue_new(void) {
	queue_t *q = kmalloc(sizeof(queue_t));
	if (!q) {
		return NULL;
	}
	q->head = q->tail = NULL;
	return q;
}

inline void queue_push(queue_t *q, tcp_conn_t *conn) {
	pending_node_t *n = kmalloc(sizeof(pending_node_t));
	if (!n) {
		return;
	}
	n->conn = conn;
	n->next = NULL;
	if (q->tail) {
		q->tail->next = n;
	} else {
		q->head = n;
	}
	q->tail = n;
}

inline tcp_conn_t* queue_pop(queue_t *q) {
	if (!q || !q->head) {
		return NULL;
	}
	pending_node_t *n = q->head;
	tcp_conn_t *c = n->conn;
	q->head = n->next;
	if (!q->head) {
		q->tail = NULL;
	}
	kfree_null(&n);
	return c;
}

inline bool queue_empty(queue_t *q) {
	return !q || !q->head;
}

inline void queue_free(queue_t *q) {
	while (!queue_empty(q)) {
		(void) queue_pop(q);
	}
	kfree_null(&q);
}
