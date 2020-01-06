#include <stdio.h>
//#include <shmem.h>

int main(void)
{
	int mype, no_pes;
	shmem_init();
	no_pes = shmem_n_pes();
	mype = shmem_my_pe();
	printf("Printing hello from %d within %d\n", mype , no_pes);
	shmem_finalize();
	return 0;
}
