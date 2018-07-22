#define _GNU_SOURCE

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include <unistd.h>
#include <sched.h>
#include<time.h>
#include<math.h>
/**
* all_cores
* Checking how much cores are available
*
*/
static cpu_set_t all_cores(void){
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
/**
* printingFoo
* @base - the number which will be printed, f.e base == 2, and the amount of that is 5
* then number 2 will be printed 5 times.
* @amount - it shows how much time the base need to be printed
*/
void printingFoo (long int base,int amount){
	for (int i =0;i<amount;i++){	
	if(i == 0)
	printf("\n%ld",base);
	if(i>0)
	printf("x%ld",base);
	
	}
}
/**
* NumFactor - function that finds the prime numbers from input num
* @Num - the number which need to be factorizated
*
*/
void NumFactor(long int Num){
long int tmp = Num;	
for(int i =2;i<=sqrt(Num);i++){
	if (Num%i == 0){
		int cnt = 0;
		while(Num%i==0){
		Num=Num/i;
		cnt++;
		}
	//printingFoo(i,cnt); /* to show more info in terminal, uncomment that*/
	}
}
//if(Num!=1)
//printingFoo(Num,1);		/* you may also uncomment that for more info*/
//printf(" that was the number : %ld\n",tmp); 
}
/**
 * timespec_diff() - returns time difference in milliseconds for two timespecs.
 * @stop:	time after event.
 * @start:	time before event.
 *
 * Uses difftime() for time_t seconds calcultation.
 */
static double timespec_diff(struct timespec *stop, struct timespec *start){
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
	//pthread_mutex_t *lock;	/* Lock for result */
	
};
/**
* threadfunc - the main func, which is used by threads
* @args - arguments (pointer for an array, size of array)
*
* We are using func NumFactor, and measure the time that was spent by the thread
*
*/
void *threadfunc(void *args){		
	struct thread_data *data = args;
	clock_gettime(CLOCK_REALTIME, &data->start_time);
	for (int i = 0; i < data->num_items; i++)
		NumFactor(data->arrptr[i]);
	clock_gettime(CLOCK_REALTIME, &data->end_time);
}

int main (int argc,char*argv[]){
int num_threads = 0;
int numofarrays = 11;
char name_of_file[20];
long int arraysize[] = {5000,10000,25000,50000,100000,250000,500000,1000000,2500000,5000000,10000000};//Used const sizes of arrays 'cause i'm lazy)
if(argc == 1){
printf("Not enough arguments\n");
printf("Usage:\n%s <Amount threads> <Name_of_File> \n", argv[0]);
exit(-1);
}
if(argc > 1)
num_threads = atoi(argv[1]);
if(argc == 3)
strcpy(name_of_file,argv[2]);
if(argc>3){
printf("too much arguments \n");
printf("Usage:\n%s <Amount threads> <Name_of_File> \n", argv[0]);
exit(-1);
}
	pthread_t threads[num_threads];
	struct thread_data th_dat[num_threads];
FILE *fp_rand,*file;
fp_rand = fopen("/dev/random", "rb");
if (NULL == fp_rand) {	
		goto exc_fopen;
}	//generating random seed 
	unsigned int seed;
	fread(&seed, sizeof(seed), 1, fp_rand);
	if (ferror(fp_rand)) {
		goto exc_fread;
	}
	srand(seed);

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
	if (ret!=0){
	goto err_affinity;
	}
	file = fopen(name_of_file,"w");
if (NULL == file) 
	goto exc_fopen;

	fprintf(file,"#AmountOfElements,N		#TimeOfCalculation,ms");
/*Starting the calculations */
for (int j = 0;j<numofarrays;j++){

long int*array = malloc(arraysize[j] * sizeof *array);
	if(array == NULL)
	goto err_alloc;
	for (long long i = 0; i < arraysize[j]; i++)
		array[i] =rand();

	struct timespec time_now, time_after;
	for (int i = 0; i < num_threads; i++) {
		long long slice = arraysize[j] / num_threads;
		th_dat[i].arrptr = &(array[i * slice]);	/* Points to start of array slice */
		th_dat[i].num_items = slice;		/* Elements in slice */
		//th_dat[i].resptr = &result;		/* Pointer to result(shared) */
		//th_dat[i].lock = &sharedlock;		/* Lock for result */
		pthread_create(&threads[i], &thread_attrs,
                               &threadfunc, &th_dat[i]);
	}
for (int i = 0; i < num_threads; i++)
		pthread_join(threads[i], NULL);
	double took_avg = 0.;
	for (int i = 0; i < num_threads; i++) {
		took_avg += timespec_diff(&(th_dat[i].end_time), 
					  &(th_dat[i].start_time));	
	}
	took_avg = took_avg/num_threads;//average time of calculation the program

printf("Numbers: %ld\nThreads: %d\n"
	       "Calculation took, ms: %g\n",
	       arraysize[j], num_threads, took_avg);
	fprintf(file,"\n%ld					%g",arraysize[j],took_avg);
	free(array);
	

}
fclose(fp_rand);
fclose(file);
return 0;
err_affinity:
printf("error with setaffinity\n");
exit(-1);
exc_fread:
printf("Can't read the file, closing the program..\n");
fclose(fp_rand);
exit(-1);
exc_fopen:
printf("Can't open the file!\n");
exit(-1);
err_alloc:
printf("Can't allocate memory for array\n");
exit(-1);

//pthread_mutex_destroy(&sharedlock);

}


