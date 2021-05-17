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

#define DSI(C) \
	(C)->d[(C)->dp] = (C)->d1; \
	(C)->dp = (C)->dp + 1 & 31; \
	(C)->d1 = (C)->d0;

#define DSR(C) \
	(C)->d0 = 0; \
	(C)->d1 = 0; \
	memset((C)->d, 0, sizeof((C)->d)); \
	(C)->dp = 0;

#define CSR(C) \
	(C)->c0 = 0; \
	memset((C)->c, 0, sizeof((C)->c)); \
	(C)->cp = 0;

int
host_exec(able_host_t *host) {
	for (;;) {
		if (host == trap_data.u) {
			int q;
			q = atomic_exchange(&trap_data.q, 0);
			if (q == 1) {
				host->c.p = 0;
				DSR(&host->c);
				CSR(&host->c);
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
					case 0x85: // reset
						host->c.p = 0;
						DSR(&host->c);
						CSR(&host->c);
						break;
					case 0x86: { // depth
						uint8_t dp;
						dp = host->c.dp;
						uint8_t cp;
						cp = host->c.cp;
						DSI(&host->c);
						host->c.d0 = cp;
						DSI(&host->c);
						host->c.d0 = dp;
						break;
					}
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
