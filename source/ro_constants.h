#ifndef ROCONSTANTS_H
#define ROCONSTANTS_H

	// TODO: not hardcoded
	#define ro_ldr_r2r1_ldr_r3r1x4_str_r2r0_str_r3r0x4_bxlr ((u32)0x14004BD8)
	#define ro_svcControlProcessMemory ((u32)0x14004FA8)
	#define ro_mov_r3r7_r2r9_r1r5_r0r4_bl_svcControlProcessMemory_add_spx8_mov_r0r6_pop_r4r5r6r7r8r9r10pc ((u32)0x14003A94)
	#define ro_mov_r0r1_bx_lr = ((u32)0x14003EFA) 1
	#define ro_mov_r0r4_pop_r4pc ((u32)0x14006388)
	#define ro_pop_r1pc ((u32)0x14003D0C)
	#define ro_pop_r4lr_nop_mov_r0x3000000_vmsr_fpscrr0_bxlr ((u32)0x14001058)
	#define ro_pop_r4r5r6r7r8r9pc ((u32)0x14002604)
	#define ro_pop_r3r4r5r6r7r8r9pc ((u32)0x14003ec8)
	#define ro_pop_r4r5pc ((u32)0x140066a0)
	#define ro_pop_r4pc ((u32)0x1400638C)
	#define ro_pop_pc ((u32)0x140063d0)
	#define ro_mov_r2r5_r1r6_r0r4_memmove ((u32)0x14006310)
	#define ro_add_r0r4x4_mov_r2x80_bl_memcpy ((u32)0x140013FC)
	#define ro_svc_x27_pop_r2_str_r1r2_add_spx4_bxlr ((u32)0x140021C4)
	#define ro_pop_lr_lsls_r1r1x2_strcs_r2r0x4_bxeq_lr ((u32)0x14003d44)

#endif
