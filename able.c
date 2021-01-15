#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include "trap.h"
#include "host.h"
#include "term.h"

extern char *__progname;

void
usage() {
	fprintf(stderr, "usage: %s [-hv] [-s size] [file]\n", __progname);
	exit(1);
}

long
eatoi(const char *s) {
	long v;
	char *ep;
	v = strtol(s, &ep, 10);
	switch (ep[0]) {
		case 'G':
			v *= 1024;
		case 'M':
			v *= 1024;
		case 'K':
		case 'B':
			v *= 1024;
	}
	return v;
}

int
main(int argc, char *argv[]) {
	int v;
	v = 0;
	long bc;
	bc = 0;
	char *ifn;
	ifn = NULL;

	char c;
	while ((c = getopt(argc, argv, "hvs:")) != -1) {
		switch (c) {
			case 's':
				if (bc != 0)
					usage();
				bc = eatoi(optarg);
				break;
			case 'v':
				v++;
				break;
			case 'h':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	switch (argc) {
		case 0:
			asprintf(&ifn, "%s.img", __progname);
			break;
		case 1:
			ifn = argv[0];
			break;
		default:
			usage();
	}

	if (bc == 0) {
		struct stat ifs;
		int rc;
		rc = stat(ifn, &ifs);
		if (rc == -1)
			err(3, "%s", ifn);
		bc = ifs.st_size;
	}

	int ifd;
	ifd = open(ifn, O_RDWR);
	if (ifd == -1)
		err(3, "open");
	void *b;
	b = mmap(NULL, bc, PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (b == MAP_FAILED)
		err(3, "mmap");
	close(ifd);

	// virtual machine config

	// terminal input
	term_recv_t i0;
	memset(&i0, 0, sizeof(i0));

	able_task_t i0t;
	memset(&i0t, 0, sizeof(i0t));
	i0t.ef = (able_task_exec_t)term_recv_exec;
	i0t.t = &i0;

	able_wire_t i0w[256];
	memset(&i0w, 0, sizeof(i0w));

	// terminal output
	term_send_t o0;
	memset(&o0, 0, sizeof(o0));
	able_node_init(&o0.n);
	o0.b = malloc(1024);
	o0.bc = 1024;

	able_task_t o0t;
	memset(&o0t, 0, sizeof(o0t));
	o0t.ef = (able_task_exec_t)term_send_exec;
	o0t.t = &o0;

	able_wire_t o0w[256];
	memset(&o0w, 0, sizeof(o0w));

	// host 0
	able_node_t h0n;
	able_node_init(&h0n);

	able_port_t h0p[256];
	memset(h0p, 0, sizeof(h0p));
	able_link_t h0l[256];
	memset(h0l, 0, sizeof(h0l));
	able_host_t h0;
	memset(&h0, 0, sizeof(h0));
	h0.n = &h0n;
	h0.p = h0p;
	h0.pc = 256;
	h0.l = h0l;
	h0.lc = 256;
	h0.c.b = b;
	h0.c.bc = bc;
	h0.ts = 1000;

	able_task_t h0t;
	memset(&h0t, 0, sizeof(h0t));
	h0t.ef = (able_task_exec_t)host_exec;
	h0t.t = &h0;

	able_wire_t h0w[256];
	memset(&h0w, 0, sizeof(h0w));

	trap_data.u = &h0;
	signal(SIGINT, trap);

	// virtual network config

	// h0.p[0]<-i0.l
	able_wire_bind(&h0w[1], &h0.p[0], 1, &h0n);
	able_wire_join(&h0w[1], &i0.l);
	// o0.p<-h0.l[0]
	able_wire_bind(&o0w[0], &o0.p, 0, &o0.n);
	able_wire_join(&o0w[0], &h0.l[0]);
	// h0.p[0]<-o0.l
	able_wire_bind(&h0w[0], &h0.p[0], 0, &h0n);
	able_wire_join(&h0w[0], &o0.l);

	able_task_fork_exec(&i0t);
	able_task_fork_exec(&o0t);
	able_task_exec(&h0t);

	// should not happen
	return 2;
}
