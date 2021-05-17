#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include "host.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "trap.h"

#define P(C, Y) \
	fprintf(stderr, "%02"PRIX8"(%d) %"PRId64" %08"PRIX64" %"PRId64"/%"PRIX64" %"PRId64"/%"PRIX64" (%"PRIu8"); %"PRId64"/%"PRIX64" (%"PRIu8")\n", \
		(C)->i, \
		(Y), \
		(C)->ts, \
		(C)->p, \
		(C)->d1, (C)->d1, \
		(C)->d0, (C)->d0, \
		(C)->dp, \
		(C)->c0, (C)->c0, \
		(C)->cp);

int
host_exec(able_host_t *host) {
	for (;;) {
		able_host_t *h;
		h = trap_data.u;
		if (h == host) {
			int q;
			q = atomic_exchange(&trap_data.q, 0);
			if (q == 1) {
				h->c.p = 0;
				h->c.d0 = 0;
				h->c.d1 = 0;
				memset(h->c.d, 0, sizeof(h->c.d));
				h->c.dp = 0;
				h->c.c0 = 0;
				memset(h->c.c, 0, sizeof(h->c.c));
				h->c.cp = 0;
			}
		}
		int y;
		y = able_host_exec(host);
		switch (y) {
			case -1: // end of timeslice
				able_host_node_wait_shim(host->n, NULL, NULL);
				return y;
			case -2: // bad memory access
			case -3: // divide by zero
#ifdef DEBUG
				P(&host->c, y);
#endif
				host->c.p = 0;
				break;
			case -4: // illegal instruction
				switch (host->c.i) {
					case 0xfe: // debug
						P(&host->c, y);
						break;
					default:
#ifdef DEBUG
						P(&host->c, y);
#endif
						host->c.p = 0;
						break;
				}
				break;
			case -5: // start of timeslice
				return y;
		}
	}

	// should not happen
	return 1;
}
