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
	EXTERN    CSTACK$$Limit
	EXTERN    CSTACK$$Base

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
	MOV       A, CS                 ; Save CS register.
	MOV       X, A
	MOV       A, ES                 ; Save ES register.
	PUSH      AX
	MOVW      AX, _usCriticalNesting; Save the usCriticalNesting value.
	PUSH      AX
	MOVW      AX, _pxCurrentTCB 	; Save the Stack pointer.
	MOVW      HL, AX
	MOVW      AX, SP
	MOVW      [HL], AX
;	Switch stack pointers. Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested. On the other hand, high priority
;	interrupts which does not call FreeRTOS API functions can be nested.
	MOVW      SP, #LWRD(CSTACK$$Limit)     ; Set stack pointer
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portRESTORE_CONTEXT MACRO
;   Restores the task Stack Pointer then use this to restore usCriticalNesting,
;   general purpose registers and the CS and ES registers of the selected task
;   from the task stack
;------------------------------------------------------------------------------
portRESTORE_CONTEXT MACRO
	MOVW      AX, _pxCurrentTCB	    ; Restore the Stack pointer.
	MOVW      HL, AX
	MOVW      AX, [HL]
	MOVW      SP, AX
	POP	      AX	                ; Restore usCriticalNesting value.
	MOVW      _usCriticalNesting, AX
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
;	have been saved as follows at the beginning of an interrupt function.
;	PUSH      AX
;	PUSH      BC
;	PUSH      DE
;	PUSH      HL
;	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
;	; MOVW    AX, 0xFFFFC           ; Save the ES and CS register.
;	; So the following code is used.
;	MOV       A, CS                 ; Save CS register.
;	MOV       X, A
;	MOV       A, ES                 ; Save ES register.
;	PUSH      AX
;	Saves the context of the general purpose registers, CS and ES registers,
;	the usCriticalNesting Value and the Stack Pointer of the active Task
	MOVW      AX, _usCriticalNesting; Save the usCriticalNesting value.
	PUSH      AX
	MOVW      AX, _pxCurrentTCB     ; Save the Stack pointer.
	MOVW      HL, AX
	MOVW      AX, SP
	MOVW      [HL], AX
;	Switch stack pointers. Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested. On the other hand, high priority
;	interrupts which does not call FreeRTOS API functions can be nested.
	MOVW      SP, #LWRD(CSTACK$$Limit)     ; Set stack pointer
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portSAVE_REGISTERS_C MACRO
;   Saves the context of the general purpose registers, CS and ES registers.
;------------------------------------------------------------------------------
portSAVE_REGISTERS_C MACRO
	LOCAL     switching_sp
;	It is assumed that the general purpose registers, CS and ES registers
;	have been saved as follows at the beginning of an interrupt function.
;	PUSH      AX
;	PUSH      BC
;	PUSH      DE
;	PUSH      HL
;	; The following ICCRL78 compatible code doesn't work with Renesas RL78 simulator.
;	; MOVW    AX, 0xFFFFC           ; Save the ES and CS register.
;	; So the following code is used.
;	MOV       A, CS                 ; Save CS register.
;	MOV       X, A
;	MOV       A, ES                 ; Save ES register.
;	PUSH      AX
;	Switch stack pointers. Interrupts which call FreeRTOS API functions
;	ending with FromISR cannot be nested. On the other hand, high priority
;	When SP <= CSTACK$$Base or CSTACK$$Limit < SP, do switching.
	MOVW      AX, SP
	CMPW      AX, #LWRD(CSTACK$$Base) 
	BNH       $switching_sp
	CMPW      AX, #LWRD(CSTACK$$Limit)
	SKNH
switching_sp:
	MOVW      SP, #LWRD(CSTACK$$Limit)     ; Set stack pointer
	PUSH      AX                    ; Save the previous SP register.
	ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portRESTORE_REGISTERS MACRO
;   Restores the general purpose registers and the CS and ES registers.
;------------------------------------------------------------------------------
portRESTORE_REGISTERS MACRO
	POP       AX                    ; Restore the previous SP register.
	MOVW      SP, AX
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
