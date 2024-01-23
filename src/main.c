#define NOB_IMPLEMENTATION
#include "nob.h"

#include "platform.h"
#include "arch.h"

#if defined(_WIN32)
   #include "platform/windows.c"
#else
   #include "platform/posix.c"
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
   #include "arch/aarch64.c"
#elif defined(__x86_64__) || defined(_M_AMD64)
   #include "arch/x86_64.c"
#endif

#define JIT_MEMORY_CAP (16*1000*1000)

typedef struct
{
    size_t operand_byte_addr;
    size_t src_byte_addr;
    size_t dst_op_index;
} Backpatch;

typedef struct
{
    Backpatch *items;
    size_t count;
    size_t capacity;
} Backpatches;

typedef struct
{
    size_t *items;
    size_t count;
    size_t capacity;
} Addrs;

typedef enum
{
    OP_INC = '+',
    OP_DEC = '-',
    OP_LEFT = '<',
    OP_RIGHT = '>',
    OP_OUTPUT = '.',
    OP_INPUT = ',',
    OP_JUMP_IF_ZERO = '[',
    OP_JUMP_IF_NONZERO = ']',
} Op_Kind;

typedef struct
{
    Op_Kind kind;
    size_t operand;
} Op;

typedef struct
{
    Op *items;
    size_t count;
    size_t capacity;
} Ops;

typedef struct
{
    Nob_String_View content;
    size_t pos;
} Lexer;

bool is_bf_cmd(char ch)
{
    const char *cmds = "+-<>,.[]";
    return strchr(cmds, ch) != NULL;
}

char lexer_next(Lexer *l)
{
    while (l->pos < l->content.count && !is_bf_cmd(l->content.data[l->pos])) {
        l->pos += 1;
    }
    if (l->pos >= l->content.count) return 0;
    return l->content.data[l->pos++];
}

bool interpret(Ops ops)
{
    bool result = true;
    // TODO: there is a memory management discrepancy between interpretation and JIT.
    // Interpretation automatically extends the memory, but JIT has a fixed size memory (to simplify implementation).
    // This discrepancy should be closed somehow
    Nob_String_Builder memory = {0};
    nob_da_append(&memory, 0);
    size_t head = 0;
    size_t ip = 0;
    while (ip < ops.count) {
        Op op = ops.items[ip];
        switch (op.kind) {
            case OP_INC: {
                memory.items[head] += op.operand;
                ip += 1;
            } break;

            case OP_DEC: {
                memory.items[head] -= op.operand;
                ip += 1;
            } break;

            case OP_LEFT: {
                if (head < op.operand) {
                    printf("RUNTIME ERROR: Memory underflow");
                    nob_return_defer(false);
                }
                head -= op.operand;
                ip += 1;
            } break;

            case OP_RIGHT: {
                head += op.operand;
                while (head >= memory.count) {
                    nob_da_append(&memory, 0);
                }
                ip += 1;
            } break;

            case OP_INPUT: {
                for (size_t i = 0; i < op.operand; ++i) {
                    fread(&memory.items[head], 1, 1, stdin);
                }
                ip += 1;
            } break;

            case OP_OUTPUT: {
                for (size_t i = 0; i < op.operand; ++i) {
                    fwrite(&memory.items[head], 1, 1, stdout);
                }
                ip += 1;
            } break;

            case OP_JUMP_IF_ZERO: {
                if (memory.items[head] == 0) {
                    ip = op.operand;
                } else {
                    ip += 1;
                }
            } break;

            case OP_JUMP_IF_NONZERO: {
                if (memory.items[head] != 0) {
                    ip = op.operand;
                } else {
                    ip += 1;
                }
            } break;
        }
    }

defer:
    nob_da_free(memory);
    return result;
}

ExecutableBuffer jit_compile(Ops ops)
{
    Nob_String_Builder sb = {0};
    Backpatches backpatches = {0};
    Addrs addrs = {0};

    asm_pre(&sb);

    for (size_t i = 0; i < ops.count; ++i) {
        Op op = ops.items[i];
        nob_da_append(&addrs, sb.count);
        switch (op.kind) {
            case OP_INC: {
                asm_increment_head_value(&sb, op.operand);
            } break;

            case OP_DEC: {
                asm_decrement_head_value(&sb, op.operand);
            } break;

            // TODO: range checks for OP_LEFT and OP_RIGHT
            case OP_LEFT: {
                asm_move_head_left(&sb, op.operand);
            } break;

            case OP_RIGHT: {
                asm_move_head_right(&sb, op.operand);
            } break;

            case OP_OUTPUT: {
                for (size_t i = 0; i < op.operand; ++i) {
                    asm_write(&sb);
                }
            } break;

            case OP_INPUT: {
                for (size_t i = 0; i < op.operand; ++i) {
                    asm_read(&sb);
                }
            } break;

            case OP_JUMP_IF_ZERO: {
                asm_jmp_if_zero(&sb);
                size_t operand_byte_addr = sb.count - 4;
                size_t src_byte_addr = sb.count;

                Backpatch bp = {
                    .operand_byte_addr = operand_byte_addr,
                    .src_byte_addr = src_byte_addr,
                    .dst_op_index = op.operand,
                };

                nob_da_append(&backpatches, bp);
            } break;

            case OP_JUMP_IF_NONZERO: {
                asm_jmp_if_not_zero(&sb);
                size_t operand_byte_addr = sb.count - 4;
                size_t src_byte_addr = sb.count;

                Backpatch bp = {
                    .operand_byte_addr = operand_byte_addr,
                    .src_byte_addr = src_byte_addr,
                    .dst_op_index = op.operand,
                };

                nob_da_append(&backpatches, bp);
            } break;

            default: assert(0 && "Unreachable");
        }
    }

    nob_da_append(&addrs, sb.count);

    for (size_t i = 0; i < backpatches.count; ++i) {
        Backpatch bp = backpatches.items[i];
        int32_t src_addr = bp.src_byte_addr;
        int32_t dst_addr = addrs.items[bp.dst_op_index];
        int32_t operand = dst_addr - src_addr;

        asm_backpatch(&sb, bp.operand_byte_addr, operand);
    }

    asm_post(&sb);

    ExecutableBuffer buf = alloc_executable_buffer(sb);

    nob_da_free(sb);
    nob_da_free(backpatches);
    nob_da_free(addrs);

    return buf;
}

bool generate_ops(const char *file_path, Ops *ops)
{
    bool result = true;
    Nob_String_Builder sb = {0};
    Addrs stack = {0};

    if (!nob_read_entire_file(file_path, &sb)) {
        nob_return_defer(false);
    }
    Lexer l = {
        .content = {
            .data = sb.items,
            .count = sb.count,
        },
    };
    char c = lexer_next(&l);
    while (c) {
        switch (c) {
            case '.':
            case ',':
            case '<':
            case '>':
            case '-':
            case '+': {
                size_t count = 1;
                char s = lexer_next(&l);
                while (s == c) {
                    count += 1;
                    s = lexer_next(&l);
                }
                Op op = {
                    .kind = c,
                    .operand = count,
                };
                nob_da_append(ops, op);
                c = s;
            } break;

            case '[': {
                size_t addr = ops->count;
                Op op = {
                    .kind = c,
                    .operand = 0,
                };
                nob_da_append(ops, op);
                nob_da_append(&stack, addr);

                c = lexer_next(&l);
            } break;

            case ']': {
                if (stack.count == 0) {
                    // TODO: reports rows and columns
                    printf("%s [%zu]: ERROR: Unbalanced loop\n", file_path, l.pos);
                    nob_return_defer(false);
                }

                size_t addr = stack.items[--stack.count];
                Op op = {
                    .kind = c,
                    .operand = addr + 1,
                };
                nob_da_append(ops, op);
                ops->items[addr].operand = ops->count;

                c = lexer_next(&l);
            } break;

            default: {}
        }
    }

    if (stack.count > 0) {
        // TODO: report the location of opening unbalanced bracket
        printf("%s [%zu]: ERROR: Unbalanced loop\n", file_path, l.pos);
        nob_return_defer(false);
    }

defer:
    if (!result) {
        nob_da_free(*ops);
        memset(ops, 0, sizeof(*ops));
    }
    nob_da_free(sb);
    nob_da_free(stack);
    return result;
}

void usage(const char *program)
{
    nob_log(NOB_ERROR, "Usage: %s [--no-jit] <input.bf>", program);
}

int main(int argc, char **argv)
{
    int result = 0;
    Ops ops = {0};
    ExecutableBuffer buf = {0};
    void *memory = NULL;

    const char *program = nob_shift_args(&argc, &argv);

    bool no_jit = false;
    const char *file_path = NULL;

    while (argc > 0) {
        const char *flag = nob_shift_args(&argc, &argv);
        if (strcmp(flag, "--no-jit") == 0) {
            no_jit = true;
        } else {
            if (file_path != NULL) {
                usage(program);
                // TODO(multifile): what if we allowed providing several files and executed them sequencially
                // preserving the state of the machine between them? Maybe complicated by TODO(dead).
                nob_log(NOB_ERROR, "Providing several files is not supported");
                nob_return_defer(1);
            }

            file_path = flag;
        }
    }

    if (file_path == NULL) {
        usage(program);
        nob_log(NOB_ERROR, "No input is provided");
        nob_return_defer(1);
    }

    if (!generate_ops(file_path, &ops)) nob_return_defer(1);

    if (no_jit) {
        nob_log(NOB_INFO, "JIT: off");
        if (!interpret(ops)) nob_return_defer(1);
    } else {
        nob_log(NOB_INFO, "JIT: on");

        buf = jit_compile(ops);
        if (!buf.run) nob_return_defer(1);
        if (!memory) memory = malloc(JIT_MEMORY_CAP);
        memset(memory, 0, JIT_MEMORY_CAP);
        assert(memory != NULL);
        buf.run(memory);
    }

defer:
    nob_da_free(ops);
    free_executable_buffer(buf);
    free(memory);
    return result;
}

// TODO: Add more interesting examples.
//   Check https://brainfuck.org/ for inspiration
// TODO(dead): Dead code eliminate first loop which traditionally used as a comment.
//   May not work well if we start sequencially executing several files,
//   because consequent files may not start from the zero state.
//   See TODO(multifile).
// TODO: Optimize pattern [-] to just set the current cell to 0.
//   Probably on the level of IR.
// TODO: Windows port.
//   - [x] Platform specific mapping of executable memory
//   - [x] Platform specific stdio from JIT compiled machine code
