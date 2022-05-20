#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include <able/misc/misc.h>
#include "host.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include "trap.h"

#define DSO ABLE_MISC_CORE_DSO
#define DSI ABLE_MISC_CORE_DSI

#define DS0 ABLE_MISC_CORE_DSV(&host->c, 1)
#define DS1 ABLE_MISC_CORE_DSV(&host->c, 2)
#define CS0 ABLE_MISC_CORE_CSV(&host->c, 1)

#define DSR(C) \
	(C)->dp = 0;

#define CSR(C) \
	(C)->cp = 0;

#define T(C, Y) { \
		int y; \
		y = (Y); \
		if (DSO(C, 2)) { \
			DSR(C); \
			CSR(C); \
			y = 9; \
		} \
		DSI(C); \
		DS0 = host->c.p - 1; \
		DSI(C); \
		DS0 = y; \
		host->c.p = 0; \
	}

#define P(C, Y) { \
		int64_t d0; \
		d0 = (C)->dp < 1 ? 0 : DS0; \
		int64_t d1; \
		d1 = (C)->dp < 2 ? 0 : DS1; \
		uint64_t c0; \
		c0 = (C)->cp < 1 ? 0 : CS0; \
		fprintf(stderr, "%02"PRIX8"(%d) %"PRId64" %08"PRIX64" %"PRId64"/%"PRIX64" %"PRId64"/%"PRIX64" (%"PRIu16"); %"PRId64"/%"PRIX64" (%"PRIu16")\n", \
			(C)->i, \
			(Y), \
			(C)->ts, \
			(C)->p, \
			d1, d1, \
			d0, d0, \
			(C)->dp, \
			c0, c0, \
			(C)->cp); \
	}

int
host_init(able_misc_host_t *host) {
	T(&host->c, 1);
	return 0;
}

int
host_exec(able_misc_host_t *host) {
	for (;;) {
		if (host == trap_data.u) {
			int q;
			q = atomic_exchange(&trap_data.q, 0);
			if (q == 1) {
				DSR(&host->c);
				CSR(&host->c);
				T(&host->c, 0);
			}
		}
		int y;
		y = able_misc_host_exec(host);
		switch (y) {
			case -1: // end of timeslice
				able_misc_host_node_wait_shim(host->n, NULL, NULL);
				return y;
			case -2: // bad memory access
				T(&host->c, 2);
				break;
			case -3: // divide by zero
				T(&host->c, 3);
				break;
			case -4: // illegal instruction
				switch (host->c.i) {
					case 0x84: { // now ( - t)
						if (DSO(&host->c, 1))
							return -7;
						struct timespec ts;
						clock_gettime(CLOCK_REALTIME, &ts);
						DSI(&host->c);
						DS0 = ts.tv_sec * 1000000000 + ts.tv_nsec;
						break;
					}
					case 0x85: // reset ( - *)
						DSR(&host->c);
						CSR(&host->c);
						T(&host->c, 0);
						break;
					case 0x86: { // depth ( - u1 u2)
						if (DSO(&host->c, 2)) {
							DSR(&host->c);
							CSR(&host->c);
							T(&host->c, 7);
							break;
						}
						uint8_t dp;
						dp = host->c.dp;
						uint8_t cp;
						cp = host->c.cp;
						DSI(&host->c);
						DS0 = cp;
						DSI(&host->c);
						DS0 = dp;
						break;
					}
					case 0xfe: // debug ( - ~)
						P(&host->c, y);
						break;
					default:
#ifdef DEBUG
						P(&host->c, y);
#endif
						T(&host->c, 4);
						break;
				}
				break;
			case -5: // start of timeslice
				return y;
			case -6: // stack underflow
				DSR(&host->c);
				CSR(&host->c);
				T(&host->c, 6);
				break;
			case -7: // stack overflow
				DSR(&host->c);
				CSR(&host->c);
				T(&host->c, 7);
				break;
			case -8: // illegal register
				T(&host->c, 8);
				break;
		}
	}

	// should not happen
	return 1;
}
