#ifndef NS_H
#define NS_H

Result NS_LaunchApplicationFIRM(u64 tid, u32 flags);
Result _NS_LaunchTitle(u64 tid, u32 flags);
Result NS_LaunchFIRM(u64 tid);
Result NS_TerminateProcess(u32 pid);
Result NS_TerminateTitle(u64 tid, u64 timeout);

#endif
