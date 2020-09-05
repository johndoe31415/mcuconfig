/**
 *	mcuconfig - Generation of microcontroller build setups.
 *	Copyright (C) 2019-2020 Johannes Bauer
 *
 *	This file is part of mcuconfig.
 *
 *	mcuconfig is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; this program is ONLY licensed under
 *	version 3 of the License, later versions are explicitly excluded.
 *
 *	mcuconfig is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with mcuconfig; if not, write to the Free Software
 *	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *	Johannes Bauer <JohannesBauer@gmx.de>
**/

<%def name="cbz(register, label)">
%if dev["core"] in [ "m3", "m4" ]:
	cbz ${register}, ${label}
%else:
	# Emulating: cbz ${register}, ${label}
	cmp ${register}, #0
	beq ${label}
%endif
</%def>\
\
.syntax unified
.cpu cortex-${dev["core"]}
.fpu softvfp
.thumb

.macro _asm_memset
	# r0: destination
	# r1: pattern
	# r2: size in bytes, must be >= 0 and divisible by 4
	#ands r2, #~0x03
	${cbz("r2", "_asm_memset_end_\@")}
	subs r2, #4

	_asm_memset_loop_\@:
		str r1, [r0, r2]
		${cbz("r2", "_asm_memset_end_\@")}
		subs r2, #4
	b _asm_memset_loop_\@
	_asm_memset_end_\@:
.endm

.macro _asm_memcpy
	# r0: destination address
	# r1: source address
	# r2: size in bytes, must be >= 0 and divisible by 4
	#ands r2, #~0x03
	${cbz("r2", "_asm_memcpy_end_\@")}
	subs r2, #4

	_asm_memcpy_loop_\@:
		ldr r3, [r1, r2]
		str r3, [r0, r2]
		${cbz("r2", "_asm_memcpy_end_\@")}
		subs r2, #4
	b _asm_memcpy_loop_\@
	_asm_memcpy_end_\@:
.endm

.macro _semihosting_exit
	ldr r0, =0x18
	ldr r1, =0x20026
	bkpt #0xab
.endm

.section .text
.type Reset_Handler, %function
Reset_Handler:
	bl EarlySystemInit

	# Painting of all RAM
	ldr r0, =_sram
	ldr r1, =0xdeadbeef
	ldr r2, =_eram
	subs r2, r0
	_asm_memset

	# Load .data section
	ldr r0, =_sdata
	ldr r1, =_sidata
	ldr r2, =_edata
	subs r2, r0
	_asm_memcpy

	# Zero .bss section
	ldr r0, =_sbss
	ldr r1, =0
	ldr r2, =_ebss
	subs r2, r0
	_asm_memset

#	_semihosting_exit

	ldr r0, =0x57a0057a
	mov lr, r0
	bl SystemInit
	bl main

	_exit_loop:
	b _exit_loop
.size Reset_Handler, .-Reset_Handler
.global Reset_Handler

.section .text, "ax", %progbits
Default_Handler:
       b default_fault_handler
.size Default_Handler, .-Default_Handler
.global Default_Handler


.section .vectors, "a", %progbits
.type vectors, %object
vectors:
	.word	_eram
%for (offset, name) in vectors:
<% handler_no = (offset - 4) // 4 %>\
%if name is None:
	.word 0		// #${handler_no} at ${"0x%x" % (offset)}: Reserved
%else:
	.word ${name}_Handler		// #${handler_no} at ${"0x%x" % (offset)}
%endif
%endfor

%for (offset, name) in vectors:
%if (offset >= 8) and (name is not None):
	.weak	${name}_Handler
	.thumb_set	${name}_Handler, Default_Handler
%endif
%endfor
.size vectors, .-vectors
.global vectors
