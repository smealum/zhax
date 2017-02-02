#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "rohax2.h"
#include "zhax.h"

Result mountRomfs()
{
	u8 lowpath[0xC] = {0};

	Result ret;
	Handle romfs_handle;
	Handle fs_handle;

    if ((ret = srvGetServiceHandleDirect(&fs_handle, "fs:USER"))) return ret;
    if ((ret = FSUSER_Initialize(fs_handle))) return ret;

	FS_Path archPath = { PATH_EMPTY, 1, (u8*)"" };
	FS_Path filePath = { PATH_BINARY, sizeof(lowpath), lowpath };

	fsUseSession(fs_handle);

	ret = FSUSER_OpenFileDirectly(&romfs_handle, ARCHIVE_ROMFS, archPath, filePath, FS_OPEN_READ, 0);

	fsEndUseSession();

	if(ret) return ret;

	return romfsInitFromFile(romfs_handle, 0);
}

int main(int argc, char **argv)
{
	gfxInitDefault();

	consoleInit(GFX_TOP, NULL);

	gfxSetDoubleBuffering(GFX_BOTTOM, false);
	bottom_fb = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);

	gfxSwapBuffers();
	gfxSwapBuffers();

	Result ret = mountRomfs();
	if(ret)
	{
		printf("failed to mount romfs %08X\n", (unsigned int)ret);
		goto main_done;
	}
	
	if((ret = rohax2())) goto main_done;
	if((ret = zhax())) goto main_done;

	main_done:

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