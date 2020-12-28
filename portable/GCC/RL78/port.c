/*
 * FreeRTOS Kernel V10.4.3
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 * 1 tab == 4 spaces!
 */

/* Scheduler includes. */
#include "FreeRTOS.h"
#include "task.h"

/* The critical nesting value is initialised to a non zero value to ensure
interrupts don't accidentally become enabled before the scheduler is started. */
#define portINITIAL_CRITICAL_NESTING  ( ( uint16_t ) 10 )

/* Initial PSW value allocated to a newly created task.
 *   11000110
 *   ||||||||-------------- Carry Flag cleared
 *   |||||||--------------- In-service priority 0 Flags set to low priority
 *   |||||----------------- In-service priority 1 Flags set to low priority
 *   ||||------------------ Register bank Select 0 Flag cleared
 *   |||------------------- Auxiliary Carry Flag cleared
 *   ||-------------------- Register bank Select 1 Flag cleared
 *   |--------------------- Zero Flag set
 *   ---------------------- Global Interrupt Flag set (enabled)
 */
#define portPSW		  ( 0xc6UL )

/* Each task maintains a count of the critical section nesting depth.  Each time
a critical section is entered the count is incremented.  Each time a critical
section is exited the count is decremented - with interrupts only being
re-enabled if the count is zero.

usCriticalNesting will get set to zero when the scheduler starts, but must
not be initialised to zero as that could cause problems during the startup
sequence. */
volatile uint16_t usCriticalNesting = portINITIAL_CRITICAL_NESTING;

/*-----------------------------------------------------------*/

/*
 * Sets up the periodic ISR used for the RTOS tick using the interval timer.
 * The application writer can define configSETUP_TICK_INTERRUPT() (in
 * FreeRTOSConfig.h) such that their own tick interrupt configuration is used
 * in place of prvSetupTimerInterrupt().
 */
#ifndef configSETUP_TICK_INTERRUPT
	/* The user has not provided their own tick interrupt configuration so use
	the definition in this file (which uses the interval timer). */
	static void prvSetupTimerInterrupt( void );
#endif

/*
 * Starts the scheduler by loading the context of the first task to run.
 * (defined in portasm.S).
 */
extern void vPortStartFirstTask( void );

/*-----------------------------------------------------------*/

/*
 * Initialise the stack of a task to look exactly as if a call to
 * portSAVE_CONTEXT had been called.
 *
 * See the header file portable.h.
 */
StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack, TaskFunction_t pxCode, void *pvParameters )
{
uint32_t *pulLocal;

	/* Stack type and pointers to the stack type are both 2 bytes. */

	/* Parameters are passed in on the stack, and written using a 32bit value
	hence a space is left for the second two bytes. */
	pxTopOfStack--;

	/* Write in the parameter value. */
	pulLocal =  ( uint32_t * ) pxTopOfStack;
	*pulLocal = ( StackType_t ) pvParameters;
	pxTopOfStack--;

	/* The return address, leaving space for the first two bytes of	the
	32-bit value. */
	pxTopOfStack--;
	pulLocal = ( uint32_t * ) pxTopOfStack;
	*pulLocal = ( uint32_t ) 0;
	pxTopOfStack--;

	/* The start address / PSW value is also written in as a 32bit value,
	so leave a space for the second two bytes. */
	pxTopOfStack--;

	/* Task function start address combined with the PSW. The GNURL78 always
	uses 16 bits function pointer. */
	pulLocal = ( uint32_t * ) pxTopOfStack;
	*pulLocal = ( ( ( uint32_t ) ( uint16_t ) pxCode ) | ( portPSW << 24UL ) );
	pxTopOfStack--;

	/* Initial values for the AX, BC, DE and HL register of bank 0. */
	*pxTopOfStack = ( StackType_t ) 0x1111;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0xBCBC;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0xDEDE;
	pxTopOfStack--;
	*pxTopOfStack = ( StackType_t ) 0x2222;
	pxTopOfStack--;

	/* CS and ES registers. */
	*pxTopOfStack = ( StackType_t ) 0x0F00;
	pxTopOfStack--;

	/* The other two register banks...register bank 3 is dedicated for use
	by interrupts so is not saved as part of the task context. */
	pxTopOfStack -= 8;

	/* Finally the critical section nesting count is set to zero when the task
	first starts. */
	*pxTopOfStack = ( StackType_t ) portNO_CRITICAL_SECTION_NESTING;

	/* Return a pointer to the top of the stack that has beene generated so it
	can	be stored in the task control block for the task. */
	return pxTopOfStack;
}
/*-----------------------------------------------------------*/

portBASE_TYPE xPortStartScheduler( void )
{
	/* Setup the hardware to generate the tick.  Interrupts are disabled when
	this function is called. */
#ifdef configSETUP_TICK_INTERRUPT
	configSETUP_TICK_INTERRUPT();
#else
	prvSetupTimerInterrupt();
#endif

	/* Restore the context of the first task that is going to run. */
	vPortStartFirstTask();

	return pdTRUE;
}
/*-----------------------------------------------------------*/

void vPortEndScheduler( void )
{
	/* It is unlikely that the RL78 port will get stopped. */
}
/*-----------------------------------------------------------*/

#ifndef configSETUP_TICK_INTERRUPT
static void prvSetupTimerInterrupt( void )
{
const uint16_t usClockHz = 15000UL; /* Internal clock. */
const uint16_t usCompareMatch = ( usClockHz / configTICK_RATE_HZ ) - 1UL;

	/* Use the internal 15K clock. */
	OSMC = ( unsigned char ) 0x16;

	#if INTIT_vect == 0x38
	{
		/* Supply the interval timer clock. */
		RTCEN = ( unsigned char ) 1U;

		/* Disable INTIT interrupt. */
		ITMK = ( unsigned char ) 1;

		/* Disable ITMC operation. */
		ITMC = ( unsigned char ) 0x0000;

		/* Clear INIT interrupt. */
		ITIF = ( unsigned char ) 0;

		/* Set interval and enable interrupt operation. */
		ITMC = usCompareMatch | 0x8000U;

		/* Enable INTIT interrupt. */
		ITMK = ( unsigned char ) 0;
	}
	#endif

	#if INTIT_vect == 0x3C
	{
		/* Supply the interval timer clock. */
		TMKAEN = ( unsigned char ) 1U;

		/* Disable INTIT interrupt. */
		TMKAMK = ( unsigned char ) 1;

		/* Disable ITMC operation. */
		ITMC = ( unsigned char ) 0x0000;

		/* Clear INIT interrupt. */
		TMKAIF = ( unsigned char ) 0;

		/* Set interval and enable interrupt operation. */
		ITMC = usCompareMatch | 0x8000U;

		/* Enable INTIT interrupt. */
		TMKAMK = ( unsigned char ) 0;
	}
	#endif
}
#endif /* ifndef configSETUP_TICK_INTERRUPT */
/*-----------------------------------------------------------*/

