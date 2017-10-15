#include "my_pthread_t.h"

void testThreads(void* arg){
	int i = 0;
	while(i<10000000){
		i++;
		
	}
	printf("thread%d\n", *((int*)arg));
	return;
}

void testThreadsWithExit(){
	printf("Printing\n");
	my_pthread_exit(NULL);
	return;
}

int main(int argc, char** argv){

my_pthread_t mythread1, mythread2, mythread3, mythread4, mythread5, mythread6 = 0;
int one = 1;
int* ones = &one;
int two = 2;
int* twos = &two;
int three = 3;
int* threes = &three;
int four = 4;
int* fours = &four;
int five = 5;
int* fives = &five;
int six = 6;
int* sixs = &six;

my_pthread_create(&mythread1, NULL, (void*)&testThreads, (void*)ones);
printf("%d\n",mythread1);
my_pthread_create(&mythread2, NULL, (void*)&testThreads, (void*)twos);
printf("%d\n",mythread2);
my_pthread_create(&mythread3, NULL, (void*)&testThreads, (void*)threes);
printf("%d\n",mythread3);
my_pthread_create(&mythread4, NULL, (void*)&testThreads, (void*)fours);
printf("%d\n",mythread4);
my_pthread_create(&mythread5, NULL, (void*)&testThreads, (void*)fives);
printf("%d\n",mythread5);
my_pthread_create(&mythread6, NULL, (void*)&testThreads, (void*)sixs);
printf("%d\n",mythread6);
my_pthread_join(mythread1, NULL);
my_pthread_join(mythread2, NULL);
my_pthread_join(mythread3, NULL);
my_pthread_join(mythread4, NULL);
}