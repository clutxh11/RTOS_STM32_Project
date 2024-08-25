#include "kernel.h"
#include "main.h"
#include<stdio.h>
#include<stdbool.h>

extern void runFirstThread(void);
extern uint32_t* stackptr;
extern int __io_putchar(int ch);

uint threadNumber = 0;
uint currentThreadIndx = 0;
thread currentThread;

uint sleepNumber = 0; // new

thread threadArray[MAX_THREADS];
thread sleepingThreadArray[MAX_THREADS]; // new

void SVC_Handler_Main( unsigned int *svc_args )
	{
		unsigned int svc_number;
		/*
		* Stack contains:
		* r0, r1, r2, r3, r12, r14, the return address and xPSR
		* First argument (r0) is svc_args[0]
		*/
		svc_number = ( ( char * )svc_args[ 6 ] )[ -2 ] ;
		switch( svc_number )
		{
			case 14:
				__set_PSP((int)(uintptr_t)threadArray[(currentThreadIndx)%threadNumber].sp);
				runFirstThread();
				break;
			case 15:
				//Pend an interrupt to do the context switch
				_ICSR |= 1<<28;
				__asm("isb");
			default: /* unknown SVC */
				break;
		}
	}

void thread_function(void)
	{
		__asm("SVC #14");
	}

void osYield(void)
	{
		__asm("SVC #15");
	}

// EDF satrts

void swap(thread* a, thread* b) { // new
    thread temp = *a;
    *a = *b;
    *b = temp;
}

void pushDown(thread arr[], int n, int i) { // new
    int smallest = i;
    int leftChild = 2 * i + 1;
    int rightChild = 2 * i + 2;

    // Compare the left child's deadline with the current root's deadline
    if (leftChild < n && arr[leftChild].deadline < arr[smallest].deadline)
        smallest = leftChild;

    // Compare the right child's deadline with the current smallest deadline
    if (rightChild < n && arr[rightChild].deadline < arr[smallest].deadline)
        smallest = rightChild;

    // If the smallest is not the current root, swap them and recursively push down
    if (smallest != i) {
        swap(&arr[i], &arr[smallest]);
        pushDown(arr, n, smallest);
    }
}

void pushUp(thread arr[], int i) { // new
    int parent = (i - 1) / 2;

    // Compare the element with its parent
    if (i > 0 && arr[i].deadline < arr[parent].deadline) {
        swap(&arr[i], &arr[parent]);
        pushUp(arr, parent);
    }
}

thread popMin(thread arr[], uint* n) { // new
    if (*n <= 0) {
        thread emptyThread = {NULL, NULL, 0, 0, 0}; // Error: Heap is empty
        return emptyThread;
    }

    thread root = arr[0];
    arr[0] = arr[*n - 1]; // Swap root with the last element
    (*n)--; // Reduce the heap size
    pushDown(arr, *n, 0); // Restore heap property

    return root;
}

void insert(thread arr[], uint* n, thread value) { // new
    arr[*n] = value; // Append the value at the end
    (*n)++; // Increase the heap size
    pushUp(arr, *n - 1); // Push up the inserted value to restore heap property
}

void buildMinHeap(thread arr[], int n) { // new
    int i;
    for (i = n / 2 - 1; i >= 0; i--)
        pushDown(arr, n, i);
}

void osSched(void) // new
	{
		threadArray[0].sp = (uint32_t*)(__get_PSP() - 8*4);
		if(threadArray[0].runTime <= 0){
			threadArray[0].runTime = threadArray[0].timeSlice;
			thread popped = popMin(threadArray, &threadNumber);
			popped.sleepTime = popped.deadline;
			popped.deadline = popped.deadlineSlice;
			sleepingThreadArray[sleepNumber] = popped;
			sleepNumber++;

			__set_PSP((int)(uintptr_t)threadArray[0].sp);
		}
	}

// EDF ends

uint32_t* __stackAllocator (void)
	{
		if(threadNumber < MAX_THREADS){
			uint32_t* newstackptr = stackptr - STACK_SIZE;
			return newstackptr;
		}
		else {return NULL;}

	}

bool osThreadCreateWithDeadline(void (*func)(void*), void* args, timerStruct timerConfig)
	{
		stackptr = __stackAllocator ();
		if(stackptr == NULL) {return false;}

		*(--stackptr) = 1<<24; //A magic number, this is xPSR
		*(--stackptr) = (uint32_t)func; //the function name
		int i = 1;
		for (; i <= 14; i++) {
			if(i == 6) {
				*(--stackptr) = (uint32_t)args;
			}
			else {
				*(--stackptr) = 0xA; //An arbitrary number
			}

		}

		threadArray[threadNumber].sp = stackptr;
		threadArray[threadNumber].thread_function = func;
		threadArray[threadNumber].timeSlice = timerConfig.runTime;
		threadArray[threadNumber].runTime = timerConfig.runTime;
		threadArray[threadNumber].deadlineSlice = timerConfig.deadline;
		threadArray[threadNumber].deadline = timerConfig.deadline;
		threadArray[threadNumber].sleepTime = timerConfig.deadline;;

		threadNumber++;

		buildMinHeap(threadArray, threadNumber);

		return true;
	}

void osKernelInit (void)
	{
		threadNumber = 0;
		currentThreadIndx = 0;
		//set the priority of PendSV to almost the weakest
		SHPR3 |= 0xFE << 16; //shift the constant 0xFE 16 bits to set PendSV priority
		SHPR2 |= 0xFDU << 24; //Set the priority of SVC higher than PendSV
		SHPR3 |= 0xFE << 24;
	}


