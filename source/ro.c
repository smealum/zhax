#include <3ds.h>
#include "ro.h"

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

Result LDRRO_Shutdown(Handle* handle, void* crs_buf)
{
	u32* cmdbuf=getThreadCommandBuffer();
	cmdbuf[0]=0x00080042; //request header code
	cmdbuf[1]=(u32)crs_buf;
	cmdbuf[2]=0x0;
	cmdbuf[3]=0xffff8001;

	Result ret=0;
	if((ret=svcSendSyncRequest(*handle)))return ret;

	return cmdbuf[1];
}
