#include <stdio.h>
//#include <shmem.h>

#define N 12

	static int src[N];
int main(void)
{
	int mype, no_pes;
	int i;
	//static int src[N];
	int *temp;
	int m= N/2;
	for (i = 0; i < N; i++)
	{
		src[i] = i;
	}
	src[9] = 123456;
/*	shmem_init();
	no_pes = shmem_n_pes();
	mype = shmem_my_pe();
	temp = (int *)shmem_malloc(sizeof(*temp));
*/
	/*if (mype == 0)
	{
		//first half
		for (i = 0; i < m/2; i++)
		{
			shmem_int_get(temp, &src[i], 1, 1);
			shmem_int_put(&src[i], &src[i + m], 1, 1);
			shmem_int_put(&src[i+m], temp, 1,1);
		}
	}
	else if (mype == 1)
	{
		//second half
		for (i = m/2; i < m; i++)
		{
			shmem_int_get(temp, &src[i], 1, 1);
			shmem_int_put(&src[i], &src[i + m], 1, 1);
			shmem_int_put(&src[i + m], temp, 1, 1);
		}
			//0--->m , m+1 -->m
			// this does the swapping inpedendent of each pe
			//printf("Printing hello from %d within %d\n", mype, no_pes);
			//shmem_finalize();
	}*/
/*	for(i = (mype*m)/no_pes; i < ((mype+1)*m)/no_pes ;i++)
	{
		shmem_int_get(temp, &src[i], 1,1);
		shmem_int_put(&src[i], &src[i+m],1,1);
		shmem_int_put(&src[i+m], temp, 1,1);
	}

	shmem_barrier_all();
	printf("My pe is :%d\n", mype);
	for(i=0;i<N;i++)
	{
		printf("%d ", src[i]);
	}
	printf("\n");
*/
	shmem_finalize();
	return 0;
}



/*


libevent 



*/
