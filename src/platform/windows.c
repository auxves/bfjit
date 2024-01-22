#include <stdio.h>
#include <windows.h>
#include <io.h>

#include "nob.h"

#include "platform.h"

ExecutableBuffer alloc_executable_buffer(Nob_String_Builder sb) {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);

    ExecutableBuffer buf = {
        .len = sb.count,
        .run = VirtualAlloc(NULL, system_info.dwPageSize, MEM_COMMIT, PAGE_READWRITE),
    };

    // if (buf.run == MAP_FAILED) {
    //     nob_log(NOB_ERROR, "Could not allocate executable memory: %s", strerror(errno));
    //     return (ExecutableBuffer) {0};
    // }

    memcpy(buf.run, sb.items, buf.len);

    DWORD dummy;
    VirtualProtect(buf.run, buf.len, PAGE_EXECUTE_READ, &dummy);

    return buf;
}

void free_executable_buffer(ExecutableBuffer buf) {
    VirtualFree(buf.run, 0, MEM_RELEASE);
}

void* get_platform_write_address() {
    return _write;
}

void* get_platform_read_address() {
    return _read;
}