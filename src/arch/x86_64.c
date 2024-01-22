#include <string.h>

#include "arch.h"
#include "platform.h"

void asm_move_head_right(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_cstr(sb, "\x48\x81\xc7"); // add rdi,
    value *= 8;
    nob_da_append_many(sb, &value, sizeof(value));
}

void asm_move_head_left(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_cstr(sb, "\x48\x81\xef"); // sub rdi,
    value *= 8;
    nob_da_append_many(sb, &value, sizeof(value));
}

void asm_increment_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_cstr(sb, "\x48\x81\x07"); // add qword ptr [rdi],
    nob_da_append_many(sb, &value, sizeof(value));
}

void asm_decrement_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_cstr(sb, "\x48\x81\x2f"); // sub qword ptr [rdi],
    nob_da_append_many(sb, &value, sizeof(value));
}

void asm_write(Nob_String_Builder* sb) {
    nob_sb_append_cstr(sb, "\x57");                                         // push rdi

    nob_da_append_many(sb, "\x48\xc7\xc2\x01\x00\x00\x00", 7);              // mov rdx, 1
    nob_sb_append_cstr(sb, "\x48\x89\xfe");                                 // mov rsi, rdi
    nob_da_append_many(sb, "\x48\xc7\xc7\x01\x00\x00\x00", 7);              // mov rdi, 1

    nob_sb_append_cstr(sb, "\x48\xB8");                                     // mov rax, 
    uint64_t write_address = (uint64_t) get_platform_write_address();
    nob_da_append_many(sb, &write_address, sizeof(write_address));

    nob_sb_append_cstr(sb, "\xff\xd0");                                     // call rax

    nob_sb_append_cstr(sb, "\x5f");                                         // pop rdi
}

void asm_read(Nob_String_Builder* sb) {
    nob_sb_append_cstr(sb, "\x57");                                         // push rdi

    nob_da_append_many(sb, "\x48\xc7\xc2\x01\x00\x00\x00", 7);              // mov rdx, 1
    nob_sb_append_cstr(sb, "\x48\x89\xfe");                                 // mov rsi, rdi
    nob_da_append_many(sb, "\x48\xc7\xc7\x00\x00\x00\x00", 7);              // mov rdi, 0

    nob_sb_append_cstr(sb, "\x48\xB8");                                     // mov rax, 
    uint64_t read_address = (uint64_t) get_platform_read_address();
    nob_da_append_many(sb, &read_address, sizeof(read_address));

    nob_sb_append_cstr(sb, "\xff\xd0");                                     // call rax

    nob_sb_append_cstr(sb, "\x5f");                                         // pop rdi
}

void asm_jmp_if_zero(Nob_String_Builder* sb) {
    nob_sb_append_cstr(sb, "\x48\x8B\x07");         // mov rax, qword ptr [rdi]
    nob_sb_append_cstr(sb, "\x48\x85\xC0");         // test rax, rax
    nob_sb_append_cstr(sb, "\x0f\x84");             // jz
    nob_da_append_many(sb, "\x00\x00\x00\x00", 4);
}

void asm_jmp_if_not_zero(Nob_String_Builder* sb) {
    nob_sb_append_cstr(sb, "\x48\x8B\x07");         // mov rax, qword ptr [rdi]
    nob_sb_append_cstr(sb, "\x48\x85\xC0");         // test rax, rax
    nob_sb_append_cstr(sb, "\x0f\x85");             // jnz
    nob_da_append_many(sb, "\x00\x00\x00\x00", 4);
}

void asm_pre(Nob_String_Builder* sb) { }

void asm_post(Nob_String_Builder* sb) {
    nob_sb_append_cstr(sb, "\xC3");                 // ret
}

void asm_backpatch(Nob_String_Builder *sb, size_t src_addr, int32_t diff) {
    memcpy(&sb->items[src_addr], &diff, sizeof(diff));
}