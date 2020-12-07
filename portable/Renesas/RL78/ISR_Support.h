;/*
; * FreeRTOS Kernel V10.4.2
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

;;$include "FreeRTOSConfig.h"
	configTICK_VECTOR    .SET    0x38

; Variables used by scheduler
;------------------------------------------------------------------------------
	.EXTERN    _pxCurrentTCB
	.EXTERN    _usCriticalNesting

;------------------------------------------------------------------------------
;   portSAVE_CONTEXT MACRO
;   Saves the context of the general purpose registers, CS and ES (only in far
;	memory mode) registers the usCriticalNesting Value and the Stack Pointer
;   of the active Task onto the task stack
;------------------------------------------------------------------------------
portSAVE_CONTEXT .MACRO
	PUSH      AX                    ; Save AX Register to stack.
	PUSH      HL
	MOV       A, CS                 ; Save CS register.
	XCH       A, X
	MOV       A, ES                 ; Save ES register.
	PUSH      AX
	PUSH      DE                    ; Save the remaining general purpose registers.
	PUSH      BC
	MOVW      AX, !_usCriticalNesting   ; Save the usCriticalNesting value.
	PUSH      AX
	MOVW      AX, !_pxCurrentTCB    ; Save the Stack pointer.
	MOVW      HL, AX
	MOVW      AX, SP
	MOVW      [HL], AX
	.ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   portRESTORE_CONTEXT MACRO
;   Restores the task Stack Pointer then use this to restore usCriticalNesting,
;   general purpose registers and the CS and ES (only in far memory mode)
;   of the selected task from the task stack
;------------------------------------------------------------------------------
portRESTORE_CONTEXT .MACRO
	MOVW      AX, !_pxCurrentTCB    ; Restore the Stack pointer.
	MOVW      HL, AX
	MOVW      AX, [HL]
	MOVW      SP, AX
	POP       AX                    ; Restore usCriticalNesting value.
	MOVW      !_usCriticalNesting, AX
	POP       BC                    ; Restore the necessary general purpose registers.
	POP       DE
	POP       AX                    ; Restore the ES register.
	MOV       ES, A
	XCH       A, X                  ; Restore the CS register.
	MOV       CS, A
	POP       HL                    ; Restore general purpose register HL.
	POP       AX                    ; Restore AX.
	.ENDM
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;   call@ MACRO
;   Calls subroutine using preferred addressing mode depending on memory model
;   Note that __RL78_SMALL__ or __RL78_MEDIUM__ are not automatically defined
;   therefore these definition have to be defined by assembler -define option,
;   but __RL78_S1__, __RL78_S2__ or __RL78_S3__ are automatically defined by
;   assembler -dev option or -cpu option which (at least either) are required
;------------------------------------------------------------------------------
call@ .MACRO subroutine
$ifdef __RL78_SMALL__
	CALL      !subroutine
$else
$ ifdef __RL78_MEDIUM__
	CALL      !!subroutine
$ else
$  ifdef __RL78_S1__
	CALL      !subroutine
$  else ; __RL78_S2__ or __RL78_S3__ or not defined
	CALL      !!subroutine
$  endif
$ endif
$endif
	.ENDM
;------------------------------------------------------------------------------
