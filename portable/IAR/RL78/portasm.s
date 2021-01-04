;/*
; * FreeRTOS Kernel V10.4.3
; * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
; *
; * Permission is hereby granted, free of charge, to any person obtaining a copy of
; * this software and associated documentation files (the "Software"), to deal in
; * the Software without restriction, including without limitation the rights to
; * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
; * the Software, and to permit persons to whom the Software is furnished to do so,
; * subject to the following conditions:
; *
; * The above copyright notice and this permission notice shall be included in all
; * copies or substantial portions of the Software.
; *
; * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
; * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
; * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
; * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
; *
; * https://www.FreeRTOS.org
; * https://github.com/FreeRTOS
; *
; * 1 tab == 4 spaces!
; */

#include "ISR_Support.h"

#define CS                    0xFFFFC
#define ES                    0xFFFFD

#if INTIT_vect == 0x38

	#define ___interrupt_TICK_VECTOR ___interrupt_0x38

#endif

#if INTIT_vect == 0x3C

	#define ___interrupt_TICK_VECTOR ___interrupt_0x3C

#endif

#ifndef ___interrupt_TICK_VECTOR

	#error Neither vector 0x38 nor vector 0x3C is available for the tick interrupt.

#endif

	PUBLIC	  _vPortYield
	PUBLIC    _vPortStartFirstTask
	PUBLIC    _vPortTickISR
	PUBLIC    _vPortFreeRTOSInterruptCommonHandler_C

	PUBLIC	  ___interrupt_0x7E
	PUBLIC	  ___interrupt_TICK_VECTOR

	EXTERN    _vTaskSwitchContext
	EXTERN    _xTaskIncrementTick

; FreeRTOS yield handler.  This is installed as the BRK software interrupt
; handler.
    RSEG CODE:CODE
_vPortYield:
___interrupt_0x7E:
	clr1	psw.1			        ; Mask the tick interrupt and interrupts which
							        ; call FreeRTOS API functions ending with FromISR.
	ei						        ; Re-enable high priority interrupts but which
							        ; cannot call FreeRTOS API functions in ISR.
	portSAVE_CONTEXT		        ; Save the context of the current task.
	call      _vTaskSwitchContext   ; Call the scheduler to select the next task.
	portRESTORE_CONTEXT		        ; Restore the context of the next task to run.
	retb


; Starts the scheduler by restoring the context of the task that will execute
; first.
    RSEG CODE:CODE
_vPortStartFirstTask:
	portRESTORE_CONTEXT	            ; Restore the context of whichever task the ...
	reti					        ; An interrupt stack frame is used so the task
                                    ; is started using a RETI instruction.

; FreeRTOS tick handler.  This is installed as the interval timer interrupt
; handler.
	 RSEG CODE:CODE
_vPortTickISR:
___interrupt_TICK_VECTOR:
	ei						       ; Re-enable high priority interrupts but which
							       ; cannot call FreeRTOS API functions in ISR.
	portSAVE_CONTEXT		       ; Save the context of the current task.
	call	_xTaskIncrementTick    ; Call the timer tick function.
	cmpw	ax, #0x00
	skz
	call	_vTaskSwitchContext    ; Call the scheduler to select the next task.
	portRESTORE_CONTEXT		       ; Restore the context of the next task to run.
	reti


; Common interrupt handler.
	 RSEG CODE:CODE
_vPortFreeRTOSInterruptCommonHandler_C:
	; Argument: BC is the target interrupt handler address.
	portSAVE_CONTEXT_C		       ; Save the context of the current task.
	; Call the target interrupt handler.
	clrb	a
	mov		cs, a
	call	bc					   ; Call the target interrupt handler.
	portRESTORE_CONTEXT		       ; Restore the context of the next task to run.
	reti


      END
