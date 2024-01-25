#include "arch.h"
#include "platform.h"
#include "api.h"

static void asm_reg_imm(Nob_String_Builder* sb, uint8_t reg, uint32_t action, int32_t value) {
    nob_da_append_lit(sb, &(uint32_t){(value << 5) | reg | action});
}

static void asm_mov_constant(Nob_String_Builder* sb, uint8_t reg, uint64_t value) {
    asm_reg_imm(sb, reg, 0xD2800000, value & 0xFFFF);
    if ((value >> 16) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2A00000, (value >> 16) & 0xFFFF);
    if ((value >> 32) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2C00000, (value >> 32) & 0xFFFF);
    if ((value >> 48) & 0xFFFF) asm_reg_imm(sb, reg, 0xF2E00000, (value >> 48) & 0xFFFF);
}

void asm_move_head_right(Nob_String_Builder* sb, uint32_t value) {
    asm_mov_constant(sb, 0x0A, value*8);
    nob_sb_append_lit(sb, "\x00\x00\x0A\x8B"); // add x0, x0, x10
}

void asm_move_head_left(Nob_String_Builder* sb, uint32_t value) {
    asm_mov_constant(sb, 0x0A, value*8);
    nob_sb_append_lit(sb, "\x00\x00\x0A\xCB"); // sub x0, x0, x10
}

void asm_increment_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x09\x00\x40\xF9"); // ldr x9, [x0]
    asm_mov_constant(sb, 0x0A, value);
    nob_sb_append_lit(sb, "\x29\x01\x0A\x8B"); // add x9, x9, x10
    nob_sb_append_lit(sb, "\x09\x00\x00\xF9"); // str x9, [x0]
}

void asm_decrement_head_value(Nob_String_Builder* sb, uint32_t value) {
    nob_sb_append_lit(sb, "\x09\x00\x40\xF9"); // ldr x9, [x0]
    asm_mov_constant(sb, 0x0A, value);
    nob_sb_append_lit(sb, "\x29\x01\x0A\xCB"); // sub x9, x9, x10
    nob_sb_append_lit(sb, "\x09\x00\x00\xF9"); // str x9, [x0]
}

void asm_write(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\xE0\x7B\xBF\xA9"); // stp x0, x30, [sp, -16]!

    asm_mov_constant(sb, 9, (uint64_t)api_write_stdout);
    nob_sb_append_lit(sb, "\x20\x01\x3F\xD6"); // blr x9

    nob_sb_append_lit(sb, "\xE0\x7B\xC1\xA8"); // ldp x0, x30, [sp], 16
}

void asm_read(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\xE0\x7B\xBF\xA9"); // stp x0, x30, [sp, -16]!

    asm_mov_constant(sb, 9, (uint64_t)api_read_stdin);
    nob_sb_append_lit(sb, "\x20\x01\x3F\xD6"); // blr x9
    
    nob_sb_append_lit(sb, "\xE0\x7B\xC1\xA8"); // ldp x0, x30, [sp], 16
}

void asm_jmp_if_zero(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x09\x00\x40\xF9"); // ldr x9, [x0]
    nob_sb_append_lit(sb, "\x09\x00\x00\xB4"); // cbz x9, 0
}

void asm_jmp_if_not_zero(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\x09\x00\x40\xF9"); // ldr x9, [x0]
    nob_sb_append_lit(sb, "\x09\x00\x00\xB5"); // cbnz x9, 0
}

void asm_pre(Nob_String_Builder* sb) { }

void asm_post(Nob_String_Builder* sb) {
    nob_sb_append_lit(sb, "\xC0\x03\x5F\xD6"); // ret
}

void asm_backpatch(Nob_String_Builder *sb, size_t src_addr, int32_t diff) {
    uint32_t* x = (uint32_t*)&sb->items[src_addr];
    *x |= (diff << 3) & 0xFFFFFF;
}