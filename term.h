typedef struct {
	able_link_t l;
} term_recv_t;

int
term_recv_exec(term_recv_t *term_recv);

typedef struct {
	able_node_t n;
	able_port_t p;
	able_link_t l;
	uint8_t *b;
	size_t bc;
} term_send_t;

int
term_send_exec(term_send_t *term_send);
