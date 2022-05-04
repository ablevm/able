#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include "term.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
			while (able_mesg_link_send(&term_recv->l, &mb, sizeof(mb.t) + bc) < 0);
			bc = 0;
		}
	}

	return 0;
}

int
term_send_exec(term_send_t *term_send) {
	uint8_t *r;
	r = NULL;
	size_t rc;
	rc = 0;
	for (;;) {
		if (rc == 0) {
			int y;
			while((y = able_edge_clip(&term_send->e, term_send->b, term_send->bc)) < 0);
			if (y == 0)
				r = term_send->b;
			while((rc += able_edge_recv(&term_send->e)) == 0)
				able_node_wait(&term_send->n, &term_send->e, NULL);
		}
		able_mesg_t *m;
		if (rc < sizeof(*m))
			continue;
		m = (able_mesg_t *)r;
		r += m->sc;
		rc -= m->sc;
		struct {
			uint8_t t;
			uint8_t b[0];
		} *mb;
		mb = (void *)m->b;
		switch (mb->t) {
			case 0: { // type
				size_t bc;
				bc = m->bc - sizeof(mb->t);
				if (bc == 0)
					break;
				bool f;
				f = false;
				int i;
				for (i = 0; i < bc; i++) {
					char c;
					c = mb->b[i];
					if (c == '\n')
						f = true;
					putchar(c);
				}
				if (f)
					fflush(stdout);
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
