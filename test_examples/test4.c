#include <stdio.h>

//#include <shmem.h>

int main(void)
{
    long *src;
    long dest;
    int nextpe;
    int me, npes, i=0;

    shmem_init();
    me = shmem_my_pe();
    npes = shmem_n_pes();

    src = (long *) shmem_malloc(sizeof(*src));

    *src = me;

    nextpe = (me + 1) % npes;
   // nextpe = 2;

    shmem_barrier_all();

    for(i =0 ;i< 10;i++)
	{
		shmem_long_get(&dest, src, 1, nextpe);
	}

    printf("%d : %ld\n", me, dest);

    shmem_finalize();

    return 0;
}
