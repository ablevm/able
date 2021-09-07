#include <stdatomic.h>
#include <pthread.h>
#include <able/able.h>
#include <able/misc/misc.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
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
	fprintf(stderr, "usage: %s [-h] [-d size] [-c size] [-r size] [-m size] [-b base] [file]\n", __progname);
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
	bool dopt;
	dopt = false;
	uint16_t doptarg;
	doptarg = 32;
	bool copt;
	copt = false;
	uint16_t coptarg;
	coptarg = 32;
	bool ropt;
	ropt = false;
	uint8_t roptarg;
	roptarg = 32;
	bool mopt;
	mopt = false;
	uint64_t moptarg;
	moptarg = 0;
	bool bopt;
	bopt = false;
	uint64_t boptarg;
	boptarg = 0;

	char c;
	while ((c = getopt(argc, argv, "hd:c:r:m:b:")) != -1) {
		switch (c) {
			case 'd':
				if (dopt)
					usage();
				dopt = true;
				doptarg = eatoi(optarg);
				if (doptarg > UINT16_MAX)
					usage();
				break;
			case 'c':
				if (copt)
					usage();
				copt = true;
				coptarg = eatoi(optarg);
				if (coptarg > UINT16_MAX)
					usage();
				break;
			case 'r':
				if (ropt)
					usage();
				ropt = true;
				roptarg = eatoi(optarg);
				if (roptarg > UINT8_MAX)
					usage();
				break;
			case 'm':
				if (mopt)
					usage();
				mopt = true;
				moptarg = eatoi(optarg);
				break;
			case 'b':
				if (bopt)
					usage();
				bopt = true;
				boptarg = eatoi(optarg);
				break;
			case 'h':
			default:
				usage();
		}
	}

	argc -= optind;
	argv += optind;

	char *ifn;
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

	if (moptarg == 0) {
		struct stat ifs;
		int y;
		y = stat(ifn, &ifs);
		if (y == -1)
			err(3, "%s", ifn);
		moptarg = ifs.st_size;
	}

	if (boptarg > moptarg)
		usage();

	int ifd;
	ifd = open(ifn, O_RDWR);
	if (ifd == -1)
		err(3, "open");
	uint8_t *b;
	b = mmap(NULL, moptarg, PROT_READ|PROT_WRITE, MAP_SHARED, ifd, 0);
	if (b == MAP_FAILED)
		err(3, "mmap");
	close(ifd);

	// virtual machine config

	// terminal input
	term_recv_t i0;
	memset(&i0, 0, sizeof(i0));

	able_task_t ti0;
	memset(&ti0, 0, sizeof(ti0));
	ti0.ef = (able_task_exec_t)term_recv_exec;
	ti0.t = &i0;

	able_wire_t wi0[256];
	memset(&wi0, 0, sizeof(wi0));

	// terminal output
	void *o0b;
	o0b = malloc(1024);
	if (o0b == NULL)
		err(4, "malloc");
	term_send_t o0;
	memset(&o0, 0, sizeof(o0));
	able_node_init(&o0.n);
	o0.b = o0b;
	o0.bc = 1024;

	able_task_t to0;
	memset(&to0, 0, sizeof(to0));
	to0.ef = (able_task_exec_t)term_send_exec;
	to0.t = &o0;

	able_wire_t wo0[256];
	memset(&wo0, 0, sizeof(wo0));

	// host 0
	able_node_t h0n;
	able_node_init(&h0n);

	able_port_t h0p[256];
	memset(h0p, 0, sizeof(h0p));
	able_link_t h0l_[256];
	memset(h0l_, 0, sizeof(h0l_));
	void *h0l[256];
	for (int i = 0; i < 256; i++)
		h0l[i] = &h0l_[i];
	int64_t *h0d;
	h0d = calloc(sizeof(int64_t), doptarg);
	if (h0d == NULL)
		err(4, "calloc");
	uint64_t *h0c;
	h0c = calloc(sizeof(uint64_t), coptarg);
	if (h0c == NULL)
		err(4, "calloc");
	uint64_t *h0r;
	h0r = calloc(sizeof(uint64_t), roptarg);
	if (h0r == NULL)
		err(4, "calloc");
	able_misc_host_t h0;
	memset(&h0, 0, sizeof(h0));
	h0.n = &h0n;
	h0.p = h0p;
	h0.pc = 256;
	h0.l = h0l;
	h0.lc = 256;
	h0.c.b = &b[boptarg];
	h0.c.bc = moptarg - boptarg;
	h0.c.d = h0d;
	h0.c.dc = doptarg;
	h0.c.c = h0c;
	h0.c.cc = coptarg;
	h0.c.r = h0r;
	h0.c.rc = roptarg;
	h0.ts = 1000;
	host_init(&h0);

	able_task_t th0;
	memset(&th0, 0, sizeof(th0));
	th0.ef = (able_task_exec_t)host_exec;
	th0.t = &h0;

	able_wire_t wh0[256];
	memset(&wh0, 0, sizeof(wh0));

	trap_data.u = &h0;
	signal(SIGINT, trap);

	// virtual network config

	// h0.p[0]<-i0.l
	able_wire_bind(&wh0[1], &h0.p[0], 1, &h0n);
	able_wire_join(&wh0[1], &i0.l);
	// o0.p<-h0.l[0]
	able_wire_bind(&wo0[0], &o0.p, 0, &o0.n);
	able_wire_join(&wo0[0], h0.l[0]);
	// h0.p[0]<-o0.l
	able_wire_bind(&wh0[0], &h0.p[0], 0, &h0n);
	able_wire_join(&wh0[0], &o0.l);

	able_task_fork_exec(&ti0);
	able_task_fork_exec(&to0);
	able_task_exec(&th0);

	// should not happen
	return 2;
}
