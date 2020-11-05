struct {
	atomic_int q;
	void *u;
} trap_data;

void
trap(int kind);
