#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>

int
able_host_wait_shim(able_host_t *host, able_edge_t *edge, const struct timespec *time) {
	return able_node_wait(host->n, edge, time);
}

int
able_link_post_shim(able_link_t *link, able_edge_t *edge) {
	return able_node_post(link->n, edge);
}
