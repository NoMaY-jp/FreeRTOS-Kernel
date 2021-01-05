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

#include "FreeRTOSConfig.h"
#include "ISR_Support.h"

	portPSW_ISP_SYSCALL_INTERRUPT_DISABLE = (2 << 1)

	.global    _vPortYield
	.global    _vPortStartFirstTask
	.global    _vPortTickISR
	.global    _vPortFreeRTOSInterruptCommonHandler_C

	.extern    _vTaskSwitchContext
	.extern    _xTaskIncrementTick

	.section   .lowtext
	.align 2

/* FreeRTOS yield handler.  This is installed as the BRK software interrupt
handler. */
_vPortYield:
	/* Re-enable high priority interrupts but which cannot
	 * call FreeRTOS API functions ending with FromISR. */
	ei
	/* Mask the tick interrupt and user interrupts which
	 * call FreeRTOS API functions ending with FromISR
	 * while the kernel structures are being accessed and
	 * FreeRTOS interrupt dedicated stack is being used. */
	push	ax
	mov		a, psw
	and		a, #0b11111001
	or		a, #portPSW_ISP_SYSCALL_INTERRUPT_DISABLE
	mov		psw, a
	pop		ax
	/* Save the context of the current task. */
	portSAVE_CONTEXT
	/* Call the scheduler to select the next task. */
	call      !!_vTaskSwitchContext
	/* Restore the context of the next task to run. */
	portRESTORE_CONTEXT
	retb

/* Starts the scheduler by restoring the context of the task that will execute
first. */
	.align 2
_vPortStartFirstTask:
	/* Restore the context of whichever task will execute first. */
	portRESTORE_CONTEXT
	/* An interrupt stack frame is used so the task is started using RETI. */
	reti

/* FreeRTOS tick handler.  This is installed as the interval timer interrupt
handler. */
	.align 2
_vPortTickISR:
	/* Re-enable high priority interrupts but which
	 * cannot call FreeRTOS API functions in ISR. */
	ei
	/* Save the context of the currently executing task. */
	portSAVE_CONTEXT
	/* Call the RTOS tick function. */
	call      !!_xTaskIncrementTick
	/* Select the next task to run. */
	clrw	ax
	cmpw	ax, r8
	skz
	call      !!_vTaskSwitchContext
	/* Retore the context of whichever task will run next. */
	portRESTORE_CONTEXT
	reti

/* Common interrupt handler. */
	.align 2
_vPortFreeRTOSInterruptCommonHandler_C:
	/* Argument: BC is the target interrupt handler address. */
	/* Save the context of the currently executing task. */
	portSAVE_CONTEXT_C
	/* Call the target interrupt handler. */
	call	bc
	/* Retore the context of whichever task will run next. */
	portRESTORE_CONTEXT
	reti

	.end

