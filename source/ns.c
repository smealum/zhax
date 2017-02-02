#include <3ds.h>
#include "ns.h"

Handle nsHandle = 0;

Result NS_LaunchApplicationFIRM(u64 tid, u32 flags)
{
	Result ret = 0;

	if(!nsHandle) srvGetServiceHandle(&nsHandle, "ns:s");

	printf("NS_LaunchApplicationFIRM %08X %llx\n", (unsigned int)nsHandle, (long long unsigned int)tid);

	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x000500C0;
	cmdbuf[1] = tid & 0xFFFFFFFF;
	cmdbuf[2] = (tid >> 32) & 0xFFFFFFFF;
	cmdbuf[3] = flags;
	
	if((ret = svcSendSyncRequest(nsHandle))!=0) return ret;

	printf("%08X\n", (unsigned int)cmdbuf[1]);

	return (Result)cmdbuf[1];
}

Result _NS_LaunchTitle(u64 tid, u32 flags)
{
	Result ret = 0;

	if(!nsHandle) srvGetServiceHandle(&nsHandle, "ns:s");

	printf("_NS_LaunchTitle %08X %llx\n", (unsigned int)nsHandle, (long long unsigned int)tid);

	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x000200C0;
	cmdbuf[1] = tid & 0xFFFFFFFF;
	cmdbuf[2] = (tid >> 32) & 0xFFFFFFFF;
	cmdbuf[3] = flags;
	
	if((ret = svcSendSyncRequest(nsHandle))!=0) return ret;

	printf("%08X\n", (unsigned int)cmdbuf[1]);

	return (Result)cmdbuf[1];
}

Result NS_LaunchFIRM(u64 tid)
{
	Result ret = 0;
	
	if(!nsHandle) srvGetServiceHandle(&nsHandle, "ns:s");

	printf("NS_LaunchFIRM %08X %llx\n", (unsigned int)nsHandle, (long long unsigned int)tid);

	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x000100C0;
	cmdbuf[1] = tid & 0xFFFFFFFF;
	cmdbuf[2] = (tid >> 32) & 0xFFFFFFFF;
	cmdbuf[3] = 0;

	if((ret = svcSendSyncRequest(nsHandle))!=0) return ret;

	printf("%08X\n", (unsigned int)cmdbuf[1]);

	return (Result)cmdbuf[1];
}

Result NS_TerminateProcess(u32 pid)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00040040;
	cmdbuf[1] = pid;

	if(!nsHandle) srvGetServiceHandle(&nsHandle, "ns:s");

	printf("NS_TerminateProcess %08X %08x ", (unsigned int)nsHandle, (unsigned int)pid);
	
	if((ret = svcSendSyncRequest(nsHandle))!=0) return ret;

	printf("%08X\n", (unsigned int)cmdbuf[1]);

	return (Result)cmdbuf[1];
}

Result NS_TerminateTitle(u64 tid, u64 timeout)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x00110100;
	cmdbuf[1] = tid & 0xFFFFFFFF;
	cmdbuf[2] = (tid >> 32) & 0xFFFFFFFF;
	cmdbuf[3] = timeout & 0xFFFFFFFF;
	cmdbuf[4] = (timeout >> 32) & 0xFFFFFFFF;

	if(!nsHandle) srvGetServiceHandle(&nsHandle, "ns:s");

	printf("NS_TerminateTitle %08X %llx ", (unsigned int)nsHandle, (long long unsigned int)tid);
	
	if((ret = svcSendSyncRequest(nsHandle))!=0) return ret;

	printf("%08X\n", (unsigned int)cmdbuf[1]);

	return (Result)cmdbuf[1];
}
