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

#define portPSW_ISP_SYSCALL_INTERRUPT_DISABLE (2 << 1)

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
	PUBLIC    _vPortInterruptCommonHandler_C

	PUBLIC	  ___interrupt_0x7E
	PUBLIC	  ___interrupt_TICK_VECTOR

	EXTERN    _vTaskSwitchContext
	EXTERN    _xTaskIncrementTick

; FreeRTOS yield handler.  This is installed as the BRK software interrupt
; handler.  This BRK handler is called not only outside a critical section
; but also inside a critical section.  The ISP bits value of PSW depends on
; each case but the value is saved by BRK instruction as a part of PSW and
; restored by RETB instruction as a part of PSW.  Of course, FreeRTOS is
; designed carefully that such call of the yield handler inside a critical
; section doesn't make internal data structures corrupted.
    RSEG CODE:CODE
_vPortYield:
___interrupt_0x7E:
;	The following code can be optimized using set1 and clr1 instructions as follows.
;	push	ax				        ; Mask the tick interrupt (kernel interrupt)
;	mov		a, psw			        ; and user interrupts which call FreeRTOS
;	and		a, #0xF9 /*0b11111001*/ ; API functions ending with FromISR (i.e.
;	or		a, #portPSW_ISP_SYSCALL_INTERRUPT_DISABLE ; SYSCALL interrupts) while
;	mov		psw, a			        ; the kernel structures are being accessed
;	pop		ax				        ; and interrupt dedicated stack is being used.
#if (portPSW_ISP_SYSCALL_INTERRUPT_DISABLE & 0x02 /*0b00000010*/) == 0
	clr1	psw.1
#else
	set1	psw.1
#endif
#if (portPSW_ISP_SYSCALL_INTERRUPT_DISABLE & 0x04 /*0b00000100*/) == 0
	clr1	psw.2
#else
	set1	psw.2
#endif
	portSAVE_CONTEXT		        ; Save the context of the current task.
							        ; Additionally re-enable high priority interrupts
							        ; but any FreeRTOS API functions cannot be called
							        ; in its ISRs.
	call    _vTaskSwitchContext     ; Call the scheduler to select the next task.
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
	portSAVE_CONTEXT		        ; Save the context of the current task.
							        ; Additionally re-enable high priority interrupts
							        ; but any FreeRTOS API functions cannot be called
							        ; in its ISRs.
	call 	_xTaskIncrementTick     ; Call the timer tick function.
	or		a, x			        ; Check the return value is zero or not.
	skz						        ; Skip the scheduler call if the value is zero.
	call 	_vTaskSwitchContext     ; Call the scheduler to select the next task.
	portRESTORE_CONTEXT		        ; Restore the context of the next task to run.
	reti


; Common interrupt handler.
	 RSEG CODE:CODE
_vPortFreeRTOSInterruptCommonHandler_C:
	; Argument: BC is the target interrupt handler address.
	;           DE is the stack pointer value before switching stack.
	portSAVE_CONTEXT_C		       ; Save the context of the current task.
	; Call the target interrupt handler.
	mov		cs, #0
	call	bc					   ; Call the target interrupt handler.
	portRESTORE_CONTEXT		       ; Restore the context of the next task to run.
	reti


; Common interrupt handler.
	 RSEG CODE:CODE
_vPortInterruptCommonHandler_C:
	; Argument: BC is the target interrupt handler address.
	;           DE is the stack pointer value before switching stack, or meaningless
	;           depending on the ucInterruptStackNesting value.
	portSAVE_REGISTERS_C	       ; Save the content of the registers.
	; Call the target interrupt handler.
	mov		cs, #0
	call	bc					   ; Call the target interrupt handler.
	portRESTORE_REGISTERS	       ; Restore the content of the registers.
	reti


      END
