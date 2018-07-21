#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include <unistd.h>
#include <sched.h>
#include<math.h>

static cpu_set_t all_cores(void)
{
	cpu_set_t cpuset;
	pthread_t this = pthread_self();
	pthread_getaffinity_np(this, sizeof(cpu_set_t), &cpuset);
	int numcores = sysconf(_SC_NPROCESSORS_ONLN);
	for (int id = 0; id < numcores; id++) {
		CPU_SET(id, &cpuset);
	}
	pthread_setaffinity_np(this, sizeof(cpu_set_t), &cpuset);
	return cpuset;
}
void printingFoo (long int base,int amount)
{
	for (int i =0;i<amount;i++){	
	if(i == 0)
	printf("\n%ld",base);
	if(i>0)
	printf("x%ld",base);
	
	}
}
void NumFactor(long int Num){
long int tmp = Num;	
for(int i =2;i<=sqrt(Num);i++){
	if (Num%i == 0){
		int cnt = 0;
		while(Num%i==0){
		Num=Num/i;
		cnt++;
		}
	//printingFoo(i,cnt);
	}
}
//if(Num!=1)
//printingFoo(Num,1);
//printf(" that was the number : %ld\n",tmp);
}
static double timespec_diff(struct timespec *stop, struct timespec *start)
{
	double diff = difftime(stop->tv_sec, start->tv_sec);
	diff *= 1000;
	diff += (stop->tv_nsec - start->tv_nsec) / 1e6;
	return diff;
}

struct thread_data {
	struct timespec start_time, end_time;
	long int *arrptr;		/* Points to start of array slice */
	long long num_items;	/* Elements in slice */
	//long int *resptr;		/* Pointer to result(shared) */
	pthread_mutex_t *lock;	/* Lock for result */
	
};

void *threadfunc(void *args){		
	/* Struct is passed via args at pthread_create so its type is known */
	struct thread_data *data = args;

	/* We check the time spent in each thread and the global time */
	clock_gettime(CLOCK_REALTIME, &data->start_time);

	for (int i = 0; i < data->num_items; i++){
		//pthread_mutex_lock(data->lock);
		NumFactor(data->arrptr[i]);
		//pthread_mutex_unlock(data->lock);
		
	}
	clock_gettime(CLOCK_REALTIME, &data->end_time);
	 /* wait till acquire */
	/* Now we own a lock */
	//*data->resptr += r;	/* manipulate the shared data */
	      /* release lock for the others */

	return 0;
}

int main (int argc,char*argv[]){
int num_threads = 0;
long long arr_size = 0;

if(argc > 2){
num_threads = atoi(argv[1]);
arr_size = atoll(argv[2]);
}
if(argc>3){
printf("too much arg \n");
return 1;
}
;
FILE *fp_rand = fopen("/dev/random", "rb");
	unsigned int seed;
	fread(&seed, sizeof(seed), 1, fp_rand);
	/*if (ferror(fp_rand)) {
		errlvl = E_FREAD;
		
	}*/
	srand(seed);
long int*array = malloc(arr_size * sizeof *array);
	for (long long i = 0; i < arr_size; i++){
		array[i] =rand();
		//printf("%ld\n",array[i]);
}


	pthread_t threads[num_threads];
	struct thread_data th_dat[num_threads];

	pthread_attr_t thread_attrs;
	pthread_attr_init(&thread_attrs); /* fill with default attributes */
	
	// Set scheduler to FIFO for spawned threads
	// This allows for less strict implementation requirements
	pthread_attr_setschedpolicy(&thread_attrs, SCHED_FIFO);
	// Set maximum priority for main and other threads
	// As long as on Linux they compete for overall system resources
	pthread_setschedprio(pthread_self(), sched_get_priority_max(SCHED_FIFO));
	struct sched_param param;
	pthread_attr_getschedparam(&thread_attrs, &param);
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	pthread_attr_setschedparam(&thread_attrs, &param);

cpu_set_t cpuset = all_cores();
	int ret = pthread_attr_setaffinity_np(&thread_attrs, sizeof(cpu_set_t), &cpuset);
	if (ret!=0)
	printf("error with setaffinity\n");

pthread_mutex_t sharedlock;
	pthread_mutex_init(&sharedlock, NULL);

	//long int result = 0;
	struct timespec time_now, time_after;
	clock_gettime(CLOCK_REALTIME, &time_now);
	for (int i = 0; i < num_threads; i++) {
		long long slice = arr_size / num_threads;
		th_dat[i].arrptr = &(array[i * slice]);	/* Points to start of array slice */
		th_dat[i].num_items = slice;		/* Elements in slice */
		//th_dat[i].resptr = &result;		/* Pointer to result(shared) */
		th_dat[i].lock = &sharedlock;		/* Lock for result */
		pthread_create(&threads[i], &thread_attrs,
                               &threadfunc, &th_dat[i]);
	}
for (int i = 0; i < num_threads; i++)
		pthread_join(threads[i], NULL);

clock_gettime(CLOCK_REALTIME, &time_after);
	double took_global = timespec_diff(&time_after, &time_now);
	double took_avg = 0.;
	for (int i = 0; i < num_threads; i++) {
		took_avg += timespec_diff(&(th_dat[i].end_time), 
					  &(th_dat[i].start_time));	
	}
	took_avg = took_avg/num_threads;

printf("Numbers: %lld\nThreads: %d\n"
	       "Average thread time, ms: %g\nCalculation took, ms: %g\n", 
	       arr_size, num_threads, took_avg, took_global);
	
	pthread_mutex_destroy(&sharedlock);
	free(array);
	fclose(fp_rand);

}
