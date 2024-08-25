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

thread threadArray[MAX_THREADS];

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

void osSched(void)
	{

		threadArray[(currentThreadIndx)%threadNumber].sp = (uint32_t*)(__get_PSP() - 8*4);
		currentThreadIndx = (currentThreadIndx + 1)%threadNumber;
		__set_PSP((int)(uintptr_t)threadArray[(currentThreadIndx)%threadNumber].sp);
	}

uint32_t* __stackAllocator (void)
	{
		if(threadNumber < MAX_THREADS){
			uint32_t* newstackptr = stackptr - STACK_SIZE;
			return newstackptr;
		}
		else {return NULL;}

	}

bool osCreateThread (void (*func)(void*), void* args)
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
		threadArray[threadNumber].timeSlice = 2000;
		threadArray[threadNumber].runTime = 2000;

		threadNumber++;

		return true;
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
		threadArray[threadNumber].timeSlice = timerConfig.timeSlice;
		threadArray[threadNumber].runTime = timerConfig.runTime;

		threadNumber++;

		return true;
	}

void osKernelInit (void)
	{
		threadNumber = 0;
		currentThreadIndx = 0;
		//set the priority of PendSV to almost the weakest
		SHPR3 |= 0xFE << 16; //shift the constant 0xFE 16 bits to set PendSV priority
		SHPR2 |= 0xFDU << 24; //Set the priority of SVC higher than PendSV
	}


