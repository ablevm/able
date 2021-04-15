#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include <stdio.h>
#include <stdlib.h>
#include "term.h"

int
term_recv_exec(term_recv_t *term_recv) {
	struct {
		uint8_t t;
		uint8_t b[1024];
	} mb;
	int bc;
	bc = 0;
	int q;
	q = 0;
	while (q == 0) {
		int c;
		c = getchar();
		if (c == EOF && feof(stdin)) {
			q = 1;
			c = 0;
		}
		if (c != '\n')
			mb.b[bc++] = c;
		if (q == 1 || c == '\n' || bc == sizeof(mb.b)) {
			mb.t = 0;
			while (able_link_send(&term_recv->l, &mb, sizeof(mb.t) + bc) < 0);
			bc = 0;
		}
	}

	return 0;
}

int
term_send_exec(term_send_t *term_send) {
	for (;;) {
		while (able_port_clip(&term_send->p, term_send->b, term_send->bc) < 0);
		able_port_mesg_t *m;
		while ((m = able_port_recv(&term_send->p)) == NULL)
			able_node_wait(&term_send->n, &term_send->p.e, NULL);
		struct {
			uint8_t t;
			uint8_t b[0];
		} *mb;
		mb = (void *)m->b;
		switch (mb->t) {
			case 0: { // type
				size_t bc;
				bc = m->bc - sizeof(mb->t);
				int i;
				for (i = 0; i < bc; i++)
					putchar(mb->b[i]);
				break;
			}
			case 1: // bye
				exit(0);
				break;
		}
	}

	// should not happen
	return 1;
}
