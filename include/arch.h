#pragma once

#include "nob.h"

void asm_move_head_right(Nob_String_Builder *sb, uint32_t value);
void asm_move_head_left(Nob_String_Builder *sb, uint32_t value);

void asm_increment_head_value(Nob_String_Builder *sb, uint32_t value);
void asm_decrement_head_value(Nob_String_Builder *sb, uint32_t value);

void asm_jmp_if_zero(Nob_String_Builder *sb);
void asm_jmp_if_not_zero(Nob_String_Builder *sb);

void asm_write(Nob_String_Builder *sb);
void asm_read(Nob_String_Builder *sb);

void asm_pre(Nob_String_Builder *sb);
void asm_post(Nob_String_Builder *sb);

void asm_backpatch(Nob_String_Builder *sb, size_t src_addr, int32_t diff);