#ifndef ROHAX2_H
#define ROHAX2_H

Result rohax2();
Result HB_ControlProcessMemory(Handle* handle, u32 addr0, u32 addr1, u32 size, u32 type, u32 permissions);
Result _HB_FlushInvalidateCache(Handle* handle);

#endif
