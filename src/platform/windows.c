#include <stdio.h>
#include <windows.h>
#include <io.h>

#include "nob.h"

#include "platform.h"

ExecutableBuffer alloc_executable_buffer(Nob_String_Builder sb) {
    ExecutableBuffer buf = {
        .len = sb.count,
        .run = VirtualAlloc(NULL, sb.count, MEM_COMMIT, PAGE_READWRITE),
    };

    memcpy(buf.run, sb.items, buf.len);

    DWORD dummy;
    VirtualProtect(buf.run, buf.len, PAGE_EXECUTE_READ, &dummy);

    return buf;
}

void free_executable_buffer(ExecutableBuffer buf) {
    VirtualFree(buf.run, 0, MEM_RELEASE);
}

void api_write_stdout(void* buf) {
    _write(1, buf, 1);
}

void api_read_stdin(void* buf) {
    _read(0, buf, 1);
}
