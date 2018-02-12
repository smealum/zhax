#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "asm.h"
#include "text.h"

#include "stub_bin.h"
#include "firm_0_18000000_bin.h"

u32 copy_core0stub_and_run_ptr = 0;
u32 stub_pa = 0;
extern u8* bottom_fb;

Result twlhaxInit(void)
{
	u32* stub_buffer = (u32*)linearAlloc(0x100000);
	stub_pa = (u32)osConvertVirtToPhys(stub_buffer);
	memcpy(stub_buffer, stub_bin, stub_bin_size);

	void* firm_0_18000000_buffer = linearAlloc(0x200000);
	u32 firm_0_18000000_pa = (u32)osConvertVirtToPhys(firm_0_18000000_buffer);
	memcpy(firm_0_18000000_buffer, firm_0_18000000_bin, firm_0_18000000_bin_size);

	// patch stub so it knows where our section is at
	{
		int i;
		for(i = 0; i < stub_bin_size / 4; i ++)
		{
			u32* ptr = &stub_buffer[i];

			if(*ptr == 0xdeadbabe)
			{
				*ptr = firm_0_18000000_pa;
				printf("found src %d\n", i);
			}else if(*ptr == 0x0deaddad){
				*ptr = firm_0_18000000_bin_size;
				printf("found len %d\n", i);
			}
		}
	}

	return 0;
}

Result twlhaxKernel11(void)
{
	__asm__ volatile("cpsid aif");

	// arm11 firm hooking inspired by https://github.com/yellows8/3ds-totalcontrolhaxx/blob/master/3ds_arm11code.s#L884
	// not used for firmlaunchhax here though obviously

	// search for core0 stub copy-and-run code
	{
		const u32 target = 0x1FFFFC00;
		u32* wram = (u32*)0xdff80000; // WRAM
		u32 length = 0x00080000; // WRAM size

		int i;
		for(i = 0; i < length; i++)
		{
			if(wram[i] == target)
			{
				copy_core0stub_and_run_ptr = (u32)&wram[i];
				break;
			}
		}
	}

	// patch up copy_core0stub_and_run so it'll load the stub we want it to
	if(copy_core0stub_and_run_ptr)
	{
		copy_core0stub_and_run_ptr -= 0x40;
		((u32*)copy_core0stub_and_run_ptr)[0] = 0xE59F0010; // ldr r0, stub_start
		((u32*)copy_core0stub_and_run_ptr)[1] = 0xE59F1010; // ldr r1, stub_end
		((u32*)copy_core0stub_and_run_ptr)[6] = stub_pa; // stub_start (overwrites real stub which will be unused now)
		((u32*)copy_core0stub_and_run_ptr)[7] = stub_pa + stub_bin_size; // stub_end
	}

	drawHex(bottom_fb, copy_core0stub_and_run_ptr, 10, 30);

	flush_dcache();

	return 0;
}
