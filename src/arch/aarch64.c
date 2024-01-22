#include "arch.h"
#include "platform.h"

void asm_reg_imm(Nob_String_Builder* sb, uint8_t reg, uint32_t action, int32_t value) {
    uint32_t x = (value << 5) | reg | action;
    nob_da_append_many(sb, &x, sizeof(x));
}

void asm_mov_constant(Nob_String_Builder* sb, uint8_t reg, uint64_t value) {
    asm_reg_imm(sb, reg, 0xD2800000, value & 0xFFFF);
    if ((value >> 16) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2A00000, (value >> 16) & 0xFFFF);
    if ((value >> 32) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2C00000, (value >> 32) & 0xFFFF);
    if ((value >> 48) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2E00000, (value >> 48) & 0xFFFF);
}

void asm_move_head_right(Nob_String_Builder* sb, uint32_t value) {
    asm_mov_constant(sb, 0x0A, value*8);
    nob_da_append_many(sb, "\x00\x00\x0A\x8B", 4); // add x0, x0, x10
}

void asm_move_head_left(Nob_String_Builder* sb, uint32_t value) {
    asm_mov_constant(sb, 0x0A, value*8);
    nob_da_append_many(sb, "\x00\x00\x0A\xCB", 4); // sub x0, x0, x10
}

void asm_increment_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_da_append_many(sb, "\x09\x00\x40\xF9", 4); // ldr x9, [x0]
    asm_mov_constant(sb, 0x0A, value);
    nob_da_append_many(sb, "\x29\x01\x0A\x8B", 4); // add x9, x9, x10
    nob_da_append_many(sb, "\x09\x00\x00\xF9", 4); // str x9, [x0]
}

void asm_decrement_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_da_append_many(sb, "\x09\x00\x40\xF9", 4); // ldr x9, [x0]
    asm_mov_constant(sb, 0x0A, value);
    nob_da_append_many(sb, "\x29\x01\x0A\xCB", 4); // sub x9, x9, x10
    nob_da_append_many(sb, "\x09\x00\x00\xF9", 4); // str x9, [x0]
}

void asm_write(Nob_String_Builder* sb) {
    nob_da_append_many(sb, "\xFF\x43\x00\xD1", 4); // sub sp, sp, 16
    nob_da_append_many(sb, "\xE0\x7B\x00\xA9", 4); // stp x0, x30, [sp]

    nob_da_append_many(sb, "\xE1\x03\x00\xAA", 4); // mov x1, x0
    asm_mov_constant(sb, 0, 1);
    asm_mov_constant(sb, 2, 1);
    asm_mov_constant(sb, 9, (uint64_t)get_platform_write_address());
    nob_da_append_many(sb, "\x20\x01\x3F\xD6", 4); // blr x9

    nob_da_append_many(sb, "\xE0\x7B\x40\xA9", 4); // ldp x0, x30, [sp]
    nob_da_append_many(sb, "\xFF\x43\x00\x91", 4); // add sp, sp, 16
}

void asm_read(Nob_String_Builder* sb) {
    nob_da_append_many(sb, "\xFF\x43\x00\xD1", 4); // sub sp, sp, 16
    nob_da_append_many(sb, "\xE0\x7B\x00\xA9", 4); // stp x0, x30, [sp]

    nob_da_append_many(sb, "\xE1\x03\x00\xAA", 4); // mov x1, x0
    asm_mov_constant(sb, 0, 0);
    asm_mov_constant(sb, 2, 1);
    asm_mov_constant(sb, 9, (uint64_t)get_platform_read_address());
    nob_da_append_many(sb, "\x20\x01\x3F\xD6", 4); // blr x9
    
    nob_da_append_many(sb, "\xE0\x7B\x40\xA9", 4); // ldp x0, x30, [sp]
    nob_da_append_many(sb, "\xFF\x43\x00\x91", 4); // add sp, sp, 16
}

void asm_jmp_if_zero(Nob_String_Builder* sb) {
    nob_da_append_many(sb, "\x09\x00\x40\xF9", 4); // ldr x9, [x0]
    nob_da_append_many(sb, "\x09\x00\x00\xB4", 4); // cbz x9, 0
}

void asm_jmp_if_not_zero(Nob_String_Builder* sb) {
    nob_da_append_many(sb, "\x09\x00\x40\xF9", 4); // ldr x9, [x0]
    nob_da_append_many(sb, "\x09\x00\x00\xB5", 4); // cbnz x9, 0
}

void asm_pre(Nob_String_Builder* sb) { }

void asm_post(Nob_String_Builder* sb) {
    nob_da_append_many(sb, "\xC0\x03\x5F\xD6", 4); // ret
}

void asm_backpatch(Nob_String_Builder *sb, size_t src_addr, int32_t diff) {
    uint32_t* x = (uint32_t*)&sb->items[src_addr];
    *x |= (diff << 3) & 0xFFFFFF;
}