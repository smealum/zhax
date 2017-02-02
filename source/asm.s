.section ".text"
.arm
.align 4

.global flush_dcache
.type flush_dcache, %function
flush_dcache:
	mov r0, #0
	mcr p15, 0, r0, c7, c10, 0

	bx lr

.global _rfe
.type _rfe, %function
_rfe:
	rfefd r0

.global _cpsr
.type _cpsr, %function
_cpsr:
	mrs r0, CPSR
	bx lr

.global return_to_usermode
.type return_to_usermode, %function
return_to_usermode:
	mov r0, #0xffffffff
	mov r1, #0x0fffffff
	svc 0xa
