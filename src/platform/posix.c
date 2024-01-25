#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>

#include "nob.h"

#include "platform.h"

ExecutableBuffer alloc_executable_buffer(Nob_String_Builder sb) {
    ExecutableBuffer buf = {
        .len = sb.count,
        .run = mmap(NULL, sb.count, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0),
    };

    if (buf.run == MAP_FAILED) {
        nob_log(NOB_ERROR, "Could not allocate executable memory: %s", strerror(errno));
        return (ExecutableBuffer) {0};
    }

    memcpy(buf.run, sb.items, buf.len);
    mprotect(buf.run, buf.len, PROT_READ | PROT_EXEC);

    return buf;
}

void free_executable_buffer(ExecutableBuffer buf) {
    munmap(buf.run, buf.len);
}

void api_write_stdout(void* buf) {
    write(1, buf, 1);
}

void api_read_stdin(void* buf) {
    read(0, buf, 1);
}
