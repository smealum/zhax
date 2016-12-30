.section ".init"
.arm
.align 4
.global _start
.global _end

_start:
	sub sp, #0x30

		ldr r9, =0xdead0001
		# first allocate a buffer at 0x0E000000
			add r0, sp, #0x20
			ldr r1, =_start
			mov r2, #0
			mov r3, #0x4000
			# COMMIT
			mov r4, #3
			str r4, [sp]
			# RW
			mov r4, #3
			str r4, [sp, #4]
			bl svc_controlMemory

		ldr r9, =0xdead0002
		# then reprotect it as RWX
			mov r0, sp
			ldr r1, =0xffff8001
			bl svc_duplicateHandle

			ldr r0, [sp]
			ldr r1, =_start
			mov r2, #0
			mov r3, #0x4000
			# PROTECT
			mov r4, #6
			str r4, [sp]
			# RWX
			mov r4, #7
			str r4, [sp, #4]
			bl svc_controlProcessMemory

		ldr r9, =0xdead0003
		# finally copy all our shit into it and jump there
			ldr r0, =_start
			adr r1, _start
			ldr r2, =_end
			sub r2, r0
			bl memcpy

		ldr r9, =0xdead0004
		# in order to flush/invalidate cache we first need to unmap process memory
		# we do that by setting up a stack that will return to our relocated code
		# and then jumping to an ro gadget that will do the syscall for us
		# clever af, i know
			# process handle
			ldr r0, =0x0fffff0c
			ldr r0, [r0]
			# gadget
			ldr lr, =jump
			push {r4-r10,lr}
			ldr pc, =0x14002b1c
			jump:
		ldr r9, =0xdead0005

	add sp, #0x30

	ldr r9, =0xdead0006
	mov sp, #0x10000000
	blx _main
