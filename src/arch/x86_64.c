#include <string.h>

#include "arch.h"
#include "platform.h"
#include "api.h"

void asm_move_head_right(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x48\x81\xc7");                              // add rdi,
    nob_da_append_lit(sb, &(uint32_t){value*8});
}

void asm_move_head_left(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x48\x81\xef");                              // sub rdi,
    nob_da_append_lit(sb, &(uint32_t){value*8});
}

void asm_increment_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x48\x81\x07");                              // add qword ptr [rdi],
    nob_da_append_lit(sb, &(uint32_t){value});
}

void asm_decrement_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x48\x81\x2f");                              // sub qword ptr [rdi],
    nob_da_append_lit(sb, &(uint32_t){value});
}

void asm_write(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x57");                                      // push rdi

#if defined(_WIN32)
    nob_sb_append_lit(sb, "\x48\x89\xF9");                              // mov rcx, rdi
#endif

    nob_sb_append_lit(sb, "\x48\xB8");                                  // mov rax, 
    nob_da_append_lit(sb, &(uint64_t){(uint64_t)api_write_stdout});

    nob_sb_append_lit(sb, "\xff\xd0");                                  // call rax

    nob_sb_append_lit(sb, "\x5f");                                      // pop rdi
}

void asm_read(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x57");                                      // push rdi

#if defined(_WIN32)
    nob_sb_append_lit(sb, "\x48\x89\xF9", 3);                           // mov rcx, rdi
#endif

    nob_sb_append_lit(sb, "\x48\xB8");                                  // mov rax, 
    nob_da_append_lit(sb, &(uint64_t){(uint64_t)api_read_stdin});
    nob_sb_append_lit(sb, "\xff\xd0");                                  // call rax

    nob_sb_append_lit(sb, "\x5f");                                      // pop rdi
}

void asm_jmp_if_zero(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x48\x8B\x07");                              // mov rax, qword ptr [rdi]
    nob_sb_append_lit(sb, "\x48\x85\xC0");                              // test rax, rax
    nob_sb_append_lit(sb, "\x0f\x84");                                  // jz
    nob_sb_append_lit(sb, "\x00\x00\x00\x00");
}

void asm_jmp_if_not_zero(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x48\x8B\x07");                              // mov rax, qword ptr [rdi]
    nob_sb_append_lit(sb, "\x48\x85\xC0");                              // test rax, rax
    nob_sb_append_lit(sb, "\x0f\x85");                                  // jnz
    nob_sb_append_lit(sb, "\x00\x00\x00\x00");
}

void asm_pre(Nob_String_Builder* sb) {
#if defined(_WIN32)
    nob_sb_append_lit(sb, "\x48\x89\xCF");                              // mov rdi, rcx
#endif
}

void asm_post(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\xC3");                                      // ret
}

void asm_backpatch(Nob_String_Builder *sb, size_t src_addr, int32_t diff) {
    memcpy(&sb->items[src_addr], &diff, sizeof(diff));
}