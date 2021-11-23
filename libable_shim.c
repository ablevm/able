#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>

int
able_host_node_wait_shim(void *node, const able_edge_t *edge, const struct timespec *time) {
	return able_node_wait(node, edge, time);
}

int
able_host_link_send_shim(void *link, const void *data, size_t size) {
	return able_link_send(link, data, size);
}

int
able_link_node_post_shim(void *node, const able_edge_t *edge) {
	return able_node_post(node, edge);
}
