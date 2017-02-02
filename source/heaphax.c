#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "heaphax.h"

extern char* fake_heap_start;
extern char* fake_heap_end;
extern u32 __ctru_heap, __ctru_heap_size, __ctru_linear_heap, __ctru_linear_heap_size;

void __attribute__((weak)) __system_allocateHeaps() {
	u32 tmp=0;

	__ctru_heap_size = 8 * 1024 * 1024;

	// Allocate the application heap
	__ctru_heap = 0x08000000;
	svcControlMemory(&tmp, __ctru_heap, 0x0, __ctru_heap_size, MEMOP_ALLOC, MEMPERM_READ | MEMPERM_WRITE);

	// Allocate the linear heap
	svcControlMemory(&__ctru_linear_heap, 0x0, 0x0, __ctru_linear_heap_size, MEMOP_ALLOC_LINEAR, MEMPERM_READ | MEMPERM_WRITE);

	// Set up newlib heap
	fake_heap_start = (char*)__ctru_heap;
	fake_heap_end = fake_heap_start + __ctru_heap_size;

}

void allocateHeapHax(u32 size, u32** main_mapping, u32** secondary_mapping)
{
	unsigned int* linear_buffer = linearMemAlign(0x1000, 0x1000);

	GX_TextureCopy((void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

	svcSleepThread(1 * 1000 * 1000);
	GSPGPU_InvalidateDataCache((u8*)linear_buffer, 0x200);
	printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
	printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);

	void* orig_header = malloc(0x200);
	memcpy(orig_header, linear_buffer, 0x200);

	{
		Result ret = svcControlMemory((u32*)main_mapping, (u32)*main_mapping, 0, size, 0x3, 0x3);
		printf("alloc %x %x\n", (unsigned int)ret, (unsigned int)*main_mapping);

		GX_TextureCopy((void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

		svcSleepThread(1 * 1000 * 1000);
		GSPGPU_InvalidateDataCache((u8*)linear_buffer, 0x200);
		printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
		printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);
	}

	{
		memcpy(linear_buffer, orig_header, 0x200);
		GSPGPU_FlushDataCache((u8*)linear_buffer, 0x200);
		
		GX_TextureCopy((void*)linear_buffer, 0xFFFFFFFF, (void*)0x16000000, 0xFFFFFFFF, 0x1000, 0x8);
		
		svcSleepThread(1 * 1000 * 1000);
	}

	{
		if(!*secondary_mapping) *secondary_mapping = (u32*)(((u32)*main_mapping) + size);
		Result ret = svcControlMemory((u32*)secondary_mapping, (u32)*secondary_mapping, 0, size, 0x3, 0x3);
		printf("alloc %x %x\n", (unsigned int)ret, (unsigned int)*secondary_mapping);

		if(ret) *secondary_mapping = NULL;

		GX_TextureCopy((void*)0x16000000, 0xFFFFFFFF, (void*)linear_buffer, 0xFFFFFFFF, 0x1000, 0x8);

		svcSleepThread(1 * 1000 * 1000);
		GSPGPU_InvalidateDataCache((u8*)linear_buffer, 0x200);
		printf("data %08X %08X %08X %08X\n", linear_buffer[0], linear_buffer[1], linear_buffer[2], linear_buffer[3]);
		printf("data %08X %08X %08X %08X\n", linear_buffer[4], linear_buffer[5], linear_buffer[6], linear_buffer[7]);
	}

	linearFree(linear_buffer);
}
