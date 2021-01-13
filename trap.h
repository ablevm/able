typedef struct {
	atomic_int q;
	void *u;
} trap_data_t;

extern trap_data_t trap_data;

void
trap(int kind);
