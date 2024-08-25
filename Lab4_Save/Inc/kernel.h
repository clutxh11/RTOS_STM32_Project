#include<stdio.h>
#include<stdbool.h>
#include<main.h>

#define SHPR2 *(uint32_t*)0xE000ED1C //for setting SVC priority, bits 31-24
#define SHPR3 *(uint32_t*)0xE000ED20 // PendSV is bits 23-16
#define _ICSR *(uint32_t*)0xE000ED04 //This lets us trigger PendSV

#define STACK_SIZE (0x200)
#define MAX_THREADS (4000/200)
#define S_TO_MS 1000

void SVC_Handler_Main( unsigned int *svc_args );

void thread_function(void);

void osYield(void);

void osSched(void);

bool osCreateThread (void (*func)(void*), void* args);

bool osThreadCreateWithDeadline(void (*func)(void*), void* args, timerStruct timerConfig);

void osKernelInit (void);

uint32_t* __stackAllocator (void);

typedef struct k_thread
{
	uint32_t* sp; //stack pointer
	void (*thread_function)(void*); //function pointer

	uint32_t timeSlice; //number of milliseconds this thread is allowed to run for
	uint32_t runTime; //number of milliseconds that this thread has left to run, once it gets started
}thread;

extern thread threadArray[MAX_THREADS];
