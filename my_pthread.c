// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name:
// username of iLab:
// iLab Server:

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ucontext.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include "my_pthread_t.h"

/**************** Global Variables ****************/

int timerCounter = 0;	//Used to stop infinite while loop for timer
bool schedInit = FALSE;
tcb* managerThread = NULL; //this is the main thread
ucontext_t main_uctx;//main context

//running threads queue - made up out of MPQ
queueNode* headRunning = NULL;
queueNode* tailRunning = NULL;
queueNode* currentRunning = NULL; //the one currently running
queueNode* nextRunning = NULL; //the one that's next to run

//initialize array of thread pointers to null.
tcb* threads[MAX_THREADS] = {NULL};

//initialize array of mutex pointers to null.
my_pthread_mutex_t* mutexes[MAX_MUTEX] = {NULL};

//prevent linear search by having a queue of ready numbers.
nextId* headThread = NULL;
nextId* tailThread = NULL;
nextId* headMutex = NULL;
nextId* tailMutex = NULL;

//don't use until array is filled to begin with
int threadCtr = 0;
bool useThreadId = FALSE;
int mutexCtr = 0;
bool useMutexId = FALSE;


//Initialize head, tail, and counter of each queue assuming only 4 levels.
queueNode* level0Qhead = NULL;
queueNode* level0Qtail = NULL;

queueNode* level1Qhead = NULL;
queueNode* level1Qtail = NULL;

queueNode* level2Qhead = NULL;
queueNode* level2Qtail = NULL;

queueNode* level3Qhead = NULL;
queueNode* level3Qtail = NULL;

int levelCtrs[4] = {0};


/**************** Additional Methods ****************/

void testThreads(){
	printf("Printing\n");
	my_pthread_yield();
	return;
}

void testThreadsWithExit(){
	printf("Printing\n");
	my_pthread_exit(NULL);
	return;
}

//initializes and runs scheduler until no threads exist
void schedulerInit(){	
	/*
	tcb* managerThread = (tcb*)malloc(sizeof(tcb));
	managerThread->tid = -1;
	managerThread->context = main_uctx;
	currentThread = managerThread;
	*/
	
	schedInit = TRUE;
	while(schedInit){
	
		maintenanceCycle();
		runThreads();
	} 
	

}

//do some maintenance between running cycles
void maintenanceCycle(){
//		
	
	//To build new queue: level 0 gets 2^0 = 1 quantum, level 1 gets 2^1 = 2 quantum, level 2 gets 2^2 quantum, level 3 2^3 quantum, level 4 gets FIFO?
	//how many of each before maintenance cycle?
	
	//need a "maintenance cycle" method that changes around priorities:
	//Suggestions:
	//	-if in 2 for more than 5 full maintenance cycle quantum, move to 1
	//	-if in 1 for more than 5 full maintenance cycle quantum, move to 0
	//	-figure out how to avoid priority inversion.
		//if mutex waiting, do not promote?
	//	-need to figure out how to bring back in from waiting queue - what should happen with these?  
	//	Back to previous level?  Start at high? start at low?
	
	if(levelCtrs[0] + levelCtrs[1] + levelCtrs[2] + levelCtrs[3] == 0){
			schedInit = FALSE;	
	}
	else{
		createRunning(); //maybe we don't need a separate method, just put logic here.
	}
}

//create list of threads to run between maintenance cycles
void createRunning(){
	//fix this to be a better running list, I'm just putting all threads in level0 in here for now
/*	headRunning = (queueNode*)malloc(sizeof(queueNode));
	headRunning->tid = -1;
	headRunning->next = NULL;
	headRunning->ctr = 0;
	tailRunning = (queueNode*)malloc(sizeof(queueNode));
	tailRunning->tid = -1;
	tailRunning->next = NULL;
	tailRunning->ctr = 0;
	
	queueNode* templevel = level0Qhead;
	while(!templevel){
		if(headRunning->tid = -1){
			printf("created first\n");
			headRunning->tid = templevel->tid;
			tailRunning = headRunning;
		}
		else if(headRunning == tailRunning){
			printf("created second\n");
			queueNode* temp = (queueNode*)malloc(sizeof(queueNode));
			temp->tid = templevel->tid;
			temp->next = NULL;
			headRunning->next = (void*)temp;
			tailRunning = temp;
		}
		else{
			printf("created more than 2\n");
			queueNode* temp = (queueNode*)malloc(sizeof(queueNode));
			temp->tid = templevel->tid;
			temp->next = NULL;
			tailRunning->next = (void*)temp;
			tailRunning = temp;			
		}
		templevel = templevel->next;
	}
*/
	printf("Move level0Qhead over to headRunning\n");
	headRunning = level0Qhead;
	level0Qhead = NULL;
	levelCtrs[0] = 0;
	//sets it so that the first thread to run is stored in global variable.
	//maybe belongs in runThreads?
	return;
}

//Run them between cycles
void runThreads(){

	printf("Assign head running to current running\n");
	nextRunning = headRunning;
	while(nextRunning){
		printf("made it to while loop\n");
		//run the thread
		my_pthread_yield();	
		currentRunning = NULL;
		//if not preempted change status to DONE
		if(threads[nextRunning->tid] == NULL){
			printf("Done\n");
		}
		else if(threads[nextRunning->tid]->threadState == PREEMPTED){
			printf("Preempted\n");
			threads[nextRunning->tid]->threadState = ACTIVE;
			levelCtrs[threads[(nextRunning->tid)]->priority] -= 1;
		}
		else if(threads[nextRunning->tid]->threadState == YIELDED){
			printf("Yielded\n");
			threads[nextRunning->tid]->threadState = ACTIVE;
			levelCtrs[threads[(nextRunning->tid)]->priority] -= 1;
		}
		//else change status to ACTIVE
		else{
			printf("Done\n");
			threads[nextRunning->tid]->threadState = DONE;
			levelCtrs[threads[(nextRunning->tid)]->priority] -= 1;
		}
		//next becomes "current"
		nextRunning = (queueNode*)nextRunning->next; //this is not working??
		printf("restart loop\n");
	}
}

/*signal handler for timer*/
void time_handle(int signum){

	//change status to PREEMPTED
	//change priority +1 on tcb node(unless bottom level)
	//put back on tail of that new priority queue. (example in my_pthread_create())
	//swapcontext back to main
	
	int count = 0;
	printf("finished %d %d\n", signum,timerCounter);
	timerCounter++;
}

//add parameter for priority then take 2^priority and * QUANTUM
void timer(){//int priority){
	struct sigaction sigact;
	struct itimerval timer;

	memset(&sigact, 0, sizeof(sigact));
	sigact.sa_handler = &time_handle;
	sigaction (SIGVTALRM, &sigact, NULL);
	
	/*next value*/		//current timer interval? why next value?
	timer.it_interval.tv_sec = 0;		//0 seconds
	timer.it_interval.tv_usec = QUANTUM;// * (1 << priority) ;	//25 milliseconds / 1 quantum

	/*current value*/	//current timer value?
	timer.it_value.tv_sec = 0;		//0 seconds
	timer.it_value.tv_usec = QUANTUM;// * (1 << priority) ;	//25 milliseconds / 1 quantum
	
	
	setitimer (ITIMER_VIRTUAL, &timer, NULL);

	while (timerCounter<40){
	
	}
}

//add to MPQ (new thread, or back in from waiting/running), must pass in level queues.
void addMPQ(tcb* thread, queueNode** head, queueNode** tail){
	queueNode* newNode = (queueNode*)malloc(sizeof(queueNode));
	
	newNode->tid = thread->tid;
	newNode->next = NULL;
	newNode->ctr = 0; 
	
		
	if(*head == NULL){//list is empty
		printf("added to head of MPQ\n");
		*head = newNode;
		*tail = newNode;
	}
	else if((*head)->tid == (*tail)->tid){//list only has one element
		printf("added as 2nd element of MPQ\n");
		(*head)->next = newNode;
		*tail = newNode;
	}
	else{//list has more than 2 or more elements.
		printf("added to end of MPQ\n");
		(*tail)->next = newNode;
		*tail = newNode;
	}	
	return;
}

/**************** Given Methods to Override ****************/

	/**************** Thread Methods ****************/
/* create a new thread */  /* DONE */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	/* Creates a pthread that executes function. Attributes are ignored, arg is not. */

	
	my_pthread_t ID = -1;
	if (useThreadId == FALSE){
		printf("use array\n");
		ID = threadCtr;
		threadCtr++;
		if(threadCtr == MAX_THREADS){
			useThreadId = TRUE;	
			headThread = (nextId*)malloc(sizeof(nextId));
			headThread->next = NULL;
			headThread->readyIndex = -1;
		}
	}
	else{
		printf("don't use array!\n");
		if(headThread->readyIndex != -1){
			ID = headThread->readyIndex;
			nextId* tempThread = headThread;
			if(headThread->next == NULL){ //this way we always have a dummy node
				headThread->readyIndex = -1;
			}
			else{
				headThread = (nextId*)headThread->next;
				free(tempThread); //frees the thread that was previously head.
			}
		}
		else{
			printf("Throw error!\n");
			//errno??
			return;
		}
	}
	printf("After\n");
	//create a new tcb
	tcb* newNode = (tcb*)malloc(sizeof(tcb));
	//assign thread ID
	newNode->tid = ID;
	if(getcontext(&(newNode->context)) == -1){
		printf("Throw error\n");
		return -1;			
	}

	//do context stuff
	ucontext_t uc = newNode->context;
	uc.uc_stack.ss_sp = (char*)malloc(STACK_SIZE);
	uc.uc_stack.ss_size = STACK_SIZE;
	uc.uc_link = &main_uctx;//should be manager thread (scheduler? maintenance?)

	//arg is the (one) argument that gets passed into function ^^
	makecontext(&uc, (void(*)(void))*function,1,arg); //THIS IS NOT WORKING!
	newNode->context = uc; 

	//assign other tcb stuff
	newNode->threadState = ACTIVE;
	newNode->priority = 0;
	newNode->retval = NULL;
	newNode->waitingThreads = NULL;
	threads[ID] = newNode;
	//"thread" is a pointer to a "buffer", store ID in that "buffer"
	*thread = ID;		
	
	//add new thread to mpq level 0
	addMPQ(threads[ID], &level0Qhead, &level0Qtail);
	levelCtrs[0] += 1;
	
	//for testing purposes, maybe we keep though?
	printf("%d\n",schedInit);
	if(!schedInit){
		schedulerInit();
	}
	return 0;
};

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	/* Explicit call to the my_pthread_t scheduler requesting that the current context can be swapped out 
	and another can be scheduled if one is waiting. */
	printf("In yield\n");
	if(currentRunning == NULL){
		currentRunning = nextRunning;
		swapcontext(&(main_uctx),&((threads[currentRunning->tid])->context));
	}
	else{
		threads[currentRunning->tid]->threadState = YIELDED;
		swapcontext(&((threads[currentRunning->tid])->context),&(main_uctx));
	}

	//always swap out of main into next, will always return back to main between.
		//swapcontext(&(where to save),&(new one to run))
	return 0;
};

/* terminate a thread */
void my_pthread_exit(void *value_ptr) {
	/* Explicit call to the my_pthread_t library to end the pthread that called it. If the value_ptr isn't 
	NULL, any return value from the thread will be saved. */
	printf("In my pthread exit\n");
	
	//before exiting, check to see if anyone else joined.
	if(threads[currentRunning->tid]->waitingThreads != NULL){
		printf("pass on a return value");
	}
	else{
		printf("No threads waiting.\n");		
	}
	//If so, pass return value on to each and put them back in ready queue.
	
	
	//Pass ID into tailThread, set curr tail = to this number.	
	if(headThread->readyIndex == -1){//none in yet
		printf("First thread ID\n");
		headThread->readyIndex = currentRunning->tid;	
		headThread->next = NULL;
		tailThread = headThread;
	}
	else if(headThread == tailThread){//only one in
		printf("Second thread ID\n");
		nextId* tmpThread = (nextId*)malloc(sizeof(nextId));
		tmpThread->readyIndex = currentRunning->tid;
		tmpThread->next = NULL;
		headThread->next = tmpThread;
		tailThread = tmpThread;
	}
	else{//two or more in
		printf("Lots in\n");
		nextId* tmpThread = (nextId*)malloc(sizeof(nextId));
		tmpThread->readyIndex = currentRunning->tid;
		tmpThread->next = NULL;
		tailThread->next = tmpThread;
		tailThread = tmpThread;
	}
	threads[currentRunning->tid] = NULL;
	
	return;
	
};

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {
	/* Call to the my_pthread_t library ensuring that the calling thread will not continue execution until the one 
	it references exits. If value_ptr is not null, the return value of the exiting thread will be passed back.*/

	//put thread onto waiting queue in tcb for thread passed in. How to get current thread ID? global var?
//	queueNode* newNode = (queueNode*)malloc(sizeof(queueNode));
//	newNode->
//	(threads[thread])->waiting = WAITING
	
	//**value_ptr is a buffer waiting for ret val. (unless not null)

	return 0;
};

	/**************** Mutex Methods ****************/
/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr) {
	/* Initializes a my_pthread_mutex_t created by the calling thread. Attributes are ignored. */
	
	//create new my_pthread_mutex_t struct.
	
	return 0;
};

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
	/* Locks a given mutex, other threads attempting to access this mutex will not run until it is unlocked. */
	//if mutex already locked, add to wait queue
	return 0;
};

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
	/* Unlocks a given mutex. */
	//check to see if anyone is waiting, if so, do not unlock, just "pass" the mutex on.  
	//Only unlock when no one else is waiting.
	return 0;
};

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	/* Destroys a given mutex. Mutex should be unlocked before doing so. */
	//check to make sure mutex is not locked first.
	return 0;
};



//need main to keep errors from occurring - use for trivial testing.
int main(int argc, char** argv){


	/* TESTING */

	printf("Calling function normally:\n");
	timer();//Still for testing purpose, obviously move to wherever needed and remove this whenever
	printf("Now trying to use a thread\n");
	timerCounter=0;
	my_pthread_t mythread1, mythread2, mythread3, mythread4, mythread5, mythread6;
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread1, NULL, (void*)&testThreads, NULL);
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread2, NULL, (void*)&testThreads, NULL);
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread3, NULL, (void*)&testThreads, NULL);
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread4, NULL, (void*)&testThreads, NULL);
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread5, NULL, (void*)&testThreadsWithExit, NULL);
	printf("%d\n",threadCtr);
	my_pthread_create(&mythread6, NULL, (void*)&testThreads, NULL);
	printf("Should NOT be 6: %d\n",threadCtr);

	queueNode* tempQ = level0Qhead;
	int count = 1;
	while(tempQ != NULL){
		printf("%d threads in queue\n", count);
		count++;
		tempQ = tempQ->next;
	}
}
