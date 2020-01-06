#include <stdio.h>
#include <time.h>
//#include <shmem.h>

#define N 9

int main()
{

	int i, nextpe, me, npes ;
	long src[N];
	long *dest;
	shmem_init();
	me = shmem_my_pe();
	npes = shmem_n_pes();

	for (i=0; i<N;i++){
		src[i] = (long)me;
	}
	dest = (long *)shmem_malloc(N*sizeof(long));
	nextpe = (me+1)%npes;
	shmem_long_put(dest , src , 9 , nextpe );
	shmem_long_put(dest , src+3 , 5 , nextpe);
	shmem_barrier_all();
	shmem_free(dest);
	shmem_finalize();
	return 0;

}
