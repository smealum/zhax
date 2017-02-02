#ifndef RO_H
#define RO_H

Result LDRRO_Initialize(Handle* handle, void* crs_buf, u32 crs_size, u32 mirror_ptr);
Result LDRRO_LoadCrr(Handle* handle, void* crr_buf, u32 crr_size);
Result LDRRO_LoadCro(Handle* handle, void* cro_buf, u32 cro_size, u32 cro_mirror, u32 data_ptr, u32 data_size, u32 bss_ptr, u32 bss_size);

#endif
