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


#include "FreeRTOSConfig.h"

; Variables used by scheduler
;------------------------------------------------------------------------------
	EXTERN    _pxCurrentTCB
	EXTERN    _usCriticalNesting
	EXTERN    _ucInterruptStackNesting
	EXTERN    _pxInterruptedTaskStack
	EXTERN    CSTACK$$Limit

;------------------------------------------------------------------------------
;   portSAVE_CONTEXT MACRO
;   Saves the context of the general purpose registers, CS and ES registers,
;   the usCriticalNesting Value and the Stack Pointer of the active Task
;   onto the task stack
;------------------------------------------------------------------------------
portSAVE_CONTEXT MACRO
	PUSH      AX                    ; Save the general purpose registers.
	PUSH      BC
	PUSH      DE
	PUSH      HL
	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
	; MOVW    AX, 0xFFFFC           ; Save the ES and CS register.
	; So the following code is used.
	MOV       A, CS                 ; Save the CS register.
	MOV       X, A
	MOV       A, ES                 ; Save the ES register.
	PUSH      AX
;	Switch stack pointers.  Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested.  On the other hand, high priority
;	interrupts which don't call any FreeRTOS API functions can be nested.
	MOVW      DE, SP
	MOVW      SP, #LWRD(CSTACK$$Limit)      ; Set stack pointer.
	ONEB      _ucInterruptStackNesting      ; Change the value: 0 --> 1.
;	Don't enable nested interrupts from the beginning of interrupt until
;	the completion of switching the stack from task stacks to interrupt
;	stack.  If it is enabled before switching the stack to interrupt
;	stack, each task stack need additional space for nested interrupt.
;	Moreover ucInterruptStackNesting has to be modified in the same DI
;	period so that the next switching of the stack is perfomed correctly.
	EI
	DECW      DE
	DECW      DE
	MOVW      AX, _usCriticalNesting; Save the usCriticalNesting value.
	MOVW      [DE], AX
	MOVW      HL, _pxCurrentTCB     ; Save the task Stack Pointer.
	MOVW      AX, DE
	MOVW      [HL], AX
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portRESTORE_CONTEXT MACRO
;   Restores the task Stack Pointer then use this to restore usCriticalNesting,
;   general purpose registers and the CS and ES registers of the selected task
;   from the task stack
;------------------------------------------------------------------------------
portRESTORE_CONTEXT MACRO
	MOVW      HL, _pxCurrentTCB	    ; Restore the task Stack Pointer.
	MOVW      AX, [HL]
;	Don't enable nested interrupts from the completion of switching the
;	stack from interrupt stack to task stacks until the RETI or RETB
;	instruction is executed.  If it is enabled after switching the stack,
;	each task stack needs additional space for nested interrupts.
;	Moreover ucInterruptStackNesting has to be modified in the same DI
;	period so that the next switching of the stack is perfomed correctly.
	DI
	MOVW      SP, AX
	CLRB      _ucInterruptStackNesting  ; Change the value: 1 --> 0.
	POP       AX
	MOVW      _usCriticalNesting, AX    ; Restore usCriticalNesting value.
	POP       AX
	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
	; MOVW    0xFFFFC, AX           ; Restore the ES and CS register.
	; So the following code is used.
	MOV       ES, A                 ; Restore the ES register.
	MOV       A, X
	MOV       CS, A                 ; Restore the CS register.
	POP       HL                    ; Restore general purpose registers.
	POP       DE
	POP       BC
	POP       AX
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portSAVE_CONTEXT_C MACRO
;   Saves the context of the general purpose registers, CS and ES registers,
;   the usCriticalNesting Value and the Stack Pointer of the active Task
;   onto the task stack
;------------------------------------------------------------------------------
portSAVE_CONTEXT_C MACRO
;	It is assumed that the general purpose registers, CS and ES registers
;	have been saved and the stack pointer has been switched as follows at
;	the beginning of an interrupt function.
;	PUSH      AX
;	PUSH      BC
;	PUSH      DE
;	PUSH      HL
;	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
;	; MOVW    AX, 0xFFFFC           ; Save the ES and CS register.
;	; So the following code is used.
;	MOV       A, CS
;	MOV       X, A
;	MOV       A, ES
;	PUSH      AX
;	Switch stack pointers.  Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested.  On the other hand, high priority
;	interrupts which don't call any FreeRTOS API functions can be nested.
;	MOVW      DE, SP
;	MOVW      SP, #LWRD(CSTACK$$Limit)
;	ONEB      !_ucInterruptStackNesting
;	Don't enable nested interrupts from the beginning of interrupt until
;	the completion of switching the stack from task stacks to interrupt
;	stack.  If it is enabled before switching the stack to interrupt
;	stack, each task stack need additional space for nested interrupt.
;	Moreover ucInterruptStackNesting has to be modified in the same DI
;	period so that the next switching of the stack is perfomed correctly.
;	EI or without EI
	DECW      DE
	DECW      DE
	MOVW      AX, _usCriticalNesting; Save the usCriticalNesting value.
	MOVW      [DE], AX
	MOVW      HL, _pxCurrentTCB     ; Save the task Stack Pointer.
	MOVW      AX, DE
	MOVW      [HL], AX
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portSAVE_REGISTERS_C MACRO
;   Saves the content of the general purpose registers, CS and ES registers.
;------------------------------------------------------------------------------
portSAVE_REGISTERS_C MACRO
;	It is assumed that the general purpose registers, CS and ES registers
;	have been saved and the stack pointer has been switched as follows at
;	the beginning of an interrupt function.
;	PUSH      AX
;	PUSH      BC
;	PUSH      DE
;	PUSH      HL
;	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
;	; MOVW    AX, 0xFFFFC           ; Save the ES and CS register.
;	; So the following code is used.
;	MOV       A, CS
;	MOV       X, A
;	MOV       A, ES
;	PUSH      AX
;	Switch stack pointers.  Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested.  On the other hand, high priority
;	interrupts which don't call any FreeRTOS API functions can be nested.
;	MOVW      DE, SP
;	CMP0      _ucInterruptStackNesting
;	SKNZ
;	MOVW      SP, #LWRD(CSTACK$$Limit)
;	INC       !_ucInterruptStackNesting
;	Don't enable nested interrupts from the beginning of interrupt until
;	the completion of switching the stack from task stacks to interrupt
;	stack.  If it is enabled before switching the stack to interrupt
;	stack, each task stack need additional space for nested interrupt.
;	Moreover ucInterruptStackNesting has to be modified in the same DI
;	period so that the next switching of the stack is perfomed correctly.
;	(On the other hand, referring ucInterruptStackNesting as follows
;	works with no problem.)
;	EI or without EI
	MOVW      AX, DE
	CMP       _ucInterruptStackNesting, #1
	SKNZ
	MOVW      _pxInterruptedTaskStack, AX   ; Save the Stack Pointer.
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portRESTORE_REGISTERS MACRO
;   Restores the general purpose registers and the CS and ES registers.
;------------------------------------------------------------------------------
portRESTORE_REGISTERS MACRO
;	Don't enable nested interrupts from the completion of switching the
;	stack from interrupt stack to task stacks until the RETI or RETB
;	instruction is executed.  If it is enabled after switching the stack,
;	each task stack needs additional space for nested interrupts.
;	Moreover ucInterruptStackNesting has to be modified in the same DI
;	period so that the next switching of the stack is perfomed correctly.
	MOVW      AX, _pxInterruptedTaskStack
	DI
	DEC       _ucInterruptStackNesting
	SKNZ
	MOVW      SP, AX                ; Restore the Stack Pointer.
	POP       AX
	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
	; MOVW    0xFFFFC, AX           ; Restore the ES and CS register.
	; So the following code is used.
	MOV       ES, A                 ; Restore the ES register.
	MOV       A, X
	MOV       CS, A                 ; Restore the CS register.
	POP       HL                    ; Restore general purpose registers.
	POP       DE
	POP       BC
	POP       AX
	ENDM
;------------------------------------------------------------------------------
