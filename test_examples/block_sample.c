#include <stdio.h>
//#include <shmem.h>

int main(void)
{
	shmem_init();
	int i, j;
	double sum =0;
	shmem_barrier_all();
	for (i =0; i < 3;i++){
		sum += i;
		shmem_barrier_all();
		sum += i;
	}
	sum += i;
	shmem_finalize();
	return 0;
}
