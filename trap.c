#include <stdatomic.h>
#include "trap.h"
#include <stdlib.h>

void
trap(int kind) {
	int q;
	q = atomic_fetch_add(&trap_data.q, 1);
	if (q == 1)
		exit(4);
}
