#pragma once

#include "nob.h"

typedef struct
{
    void (*run)(void *memory);
    size_t len;
} ExecutableBuffer;

ExecutableBuffer alloc_executable_buffer(Nob_String_Builder sb);
void free_executable_buffer(ExecutableBuffer buf);
