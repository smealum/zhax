#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctr/types.h>
#include <ctr/srv.h>
#include <ctr/svc.h>

#include "svc.h"

int _numSessionHandles = 0x3;
int* numSessionHandles = &_numSessionHandles;

// v6148; this is the only version-specific values here
Handle* sessionHandles = (Handle*)0x1400B770;

typedef void (*cmdHandlerFunction)(u32* cmdbuf);

void HB_FlushInvalidateCache(u32* cmdbuf)
{
	if(!cmdbuf)return;
	if(cmdbuf[0] != 0x10042 || cmdbuf[2] != 0)
	{
		//send error
		cmdbuf[0] = 0x00010040;
		cmdbuf[1] = 0xFFFFFFFF;
		return;
	}

	const Handle processHandle = cmdbuf[3];

	Result rc = svc_mapProcessMemory(processHandle, 0x00100000, 0x00200000);
	if(rc == 0) svc_unmapProcessMemory(processHandle, 0x00100000, 0x00200000);

	svc_closeHandle(processHandle);

	// response
	cmdbuf[0]=0x00010040;
	cmdbuf[1]=0x00000000;
}

void HB_ControlProcessMemory(u32* cmdbuf)
{
	if(!cmdbuf) return;

	if(cmdbuf[0] != 0x000A0142 || cmdbuf[6] != 0)
	{
		//send error
		cmdbuf[0] = 0x000A0040;
		cmdbuf[1] = 0xFFFFFFFF;
		return;
	}

	const Handle processHandle = cmdbuf[7];

	u32 ret = svc_controlProcessMemory(processHandle, cmdbuf[1], cmdbuf[2], cmdbuf[3], cmdbuf[4], cmdbuf[5]);

	svc_closeHandle(processHandle);

	// response
	cmdbuf[0] = 0x000A0040;
	cmdbuf[1] = ret; //error code (if any)
}

cmdHandlerFunction commandHandlers[] =
{
	HB_FlushInvalidateCache,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	HB_ControlProcessMemory
};

int _main()
{
	Result ret = 0;
	int currentHandleIndex = 2;
	u32 numCommands = sizeof(commandHandlers) / sizeof(commandHandlers[0]);
	while(1)
	{
		if(ret == 0xc920181a)
		{
			//close session handle
			svc_closeHandle(sessionHandles[currentHandleIndex]);
			sessionHandles[currentHandleIndex] = sessionHandles[*numSessionHandles];
			sessionHandles[*numSessionHandles] = 0x0;
			currentHandleIndex = (*numSessionHandles)--; //we want to have replyTarget = 0
		}else if(ret < 0)
		{
			*(u32*)(0xface0000) = ret;
		}else{
			switch(currentHandleIndex)
			{
				case 0:
					svc_exitProcess();
					break;
				case 1:
					{
						// receiving new session
						svc_acceptSession(&sessionHandles[*numSessionHandles], sessionHandles[currentHandleIndex]);
						currentHandleIndex=(*numSessionHandles)++;
					}
					break;
				default:
					{
						// receiving command from ongoing session
						u32* cmdbuf = getThreadCommandBuffer();
						u8 cmdIndex = cmdbuf[0]>>16;
						if(cmdIndex <= numCommands && cmdIndex > 0 && commandHandlers[cmdIndex-1])
						{
							commandHandlers[cmdIndex-1](cmdbuf);
						}
						else
						{
							cmdbuf[0] = (cmdbuf[0] & 0x00FF0000) | 0x40;
							cmdbuf[1] = 0xFFFFFFFF;
						}
					}
					break;
			}
		}
		ret = svc_replyAndReceive((s32*)&currentHandleIndex, sessionHandles, *numSessionHandles, sessionHandles[currentHandleIndex]);
	}

	return 0;
}
