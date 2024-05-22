#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chrono>

#include "translator.h"

extern "C" int float_printf(float* value)
{
        return printf("%f\n", *value);
}

extern "C" int float_scanf(float* value)
{
        return scanf("%f", value);
}

int AssemblyCodeAlignedInit (assembly_code* const self, const size_t alignment, const size_t bytes)
{
        self->code = (char*)aligned_alloc (alignment, bytes);

        self->size = bytes;
        self->position = 0;

        return 0;
}

int AsseblyCodeInit (assembly_code* const self, const size_t bytes)
{
        self->code = (char*)calloc (bytes, sizeof(char));

        self->size = bytes;
        self->position = 0;

        return 0;
}

int LoadBinaryCode (const char* const src_file_name, assembly_code* src_code_save)
{
        FILE* src_file = fopen (src_file_name, "rb");

        fseek (src_file, 0, SEEK_END);
        int file_size = ftell (src_file);
        fseek (src_file, 0, SEEK_SET);

        src_code_save->code = (char*)calloc (file_size, sizeof (char));
        fread (src_code_save->code, sizeof (char), file_size, src_file);
        src_code_save->size = file_size;

        fclose (src_file);

        return 0;
}

inline void WriteCommand (assembly_code* const dst_code, opcode operation_code)
{
        *(u_int64_t*)dst_code->code = operation_code.code;              // записываем в ячейку код команды
        dst_code->position         += operation_code.size;              // сдвигаем позицию в коде
        dst_code->code             += operation_code.size;              // сдвигаем адрес на следующий элемент
}

inline void WriteAsmCommand (assembly_code* const dst_code, asmcode assembler_code)
{
        memcpy (dst_code->code, assembler_code.code, assembler_code.size);
        dst_code->position += assembler_code.size;
        dst_code->size     += assembler_code.size;
        dst_code->code     += assembler_code.size;
}

inline void WriteHeader (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(header);
        strcpy (header.code, "extern scanf\nextern printf\n\nsection .data\n\nscanf_str db \"%lf\", 0\nprintf_str db \"%lf\", 10, 0\n\nsection .text\n\nglobal main\n\nmain:\nmov rbp, rsp\nsub rsp, 8  ; выравнивание на 16\n\n");
        header.size = strlen (header.code);

        WriteAsmCommand (dst_code, header);
}

inline void WriteEnd (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(end);
        strcpy (end.code, "\nEND:\n");
        end.size = strlen (end.code);

        WriteAsmCommand (dst_code, end);
}

inline void TranslatePushToAsm (assembly_code* const dst_code, const float data)
{
        INIT_ASM_ELEM(mov_rax_float);
        sprintf (mov_rax_float.code, "mov rax, __float64__(%f)  ; push imm\n", data);
        mov_rax_float.size = strlen (mov_rax_float.code);

        INIT_ASM_ELEM(mov_to_stack);
        strcpy (mov_to_stack.code, "vmovq xmm5, rax\nvmovq qword [rsp - 16], xmm5\n");
        mov_to_stack.size = strlen (mov_to_stack.code);

        INIT_ASM_ELEM(mov_and_redata);
        strcpy (mov_and_redata.code, "sub rsp, 16\nxor rax, rax\n");
        mov_and_redata.size = strlen (mov_and_redata.code);

        WriteAsmCommand (dst_code, mov_rax_float);
        WriteAsmCommand (dst_code, mov_to_stack);
        WriteAsmCommand (dst_code, mov_and_redata);

        SPACE(1);
}

inline void TranslatePush (assembly_code* const dst_code, const u_int64_t data)
{
        constexpr opcode mov_r14_imm =                                  // записываем код операции
        {
                .code = MOV_R14,                                        // код команды mov r14, imm
                .size = OPSIZE(MOV_R14)
        };

        constexpr u_int64_t double_size = sizeof(double);

        opcode write_data  =
        {
                .code = data,                                           // значение imm
                .size = double_size
        };

        constexpr opcode mov_to_stack =
        {
                .code = MOV_TO_STACK_R14,
                .size = OPSIZE(MOV_TO_STACK_R14)
        };

        constexpr opcode sub_rsp =
        {
                .code = SUB_RSP_IMM | QWORD << BYTE(3),                 // нужно проверить
                .size = OPSIZE(SUB_RSP_IMM)
        };
                                                                        // кладем все в буфер вывода
        WriteCommand (dst_code, mov_r14_imm);
        WriteCommand (dst_code, write_data);
        WriteCommand (dst_code, mov_to_stack);
        WriteCommand (dst_code, sub_rsp);
}

inline void TranslatePushRAMToAsm (assembly_code* const dst_code, const u_int64_t memory_indx)
{
        INIT_ASM_ELEM(mov_xmm5_mem);
        sprintf (mov_xmm5_mem.code, "vmovq xmm5, qword [r13 + 8 * %d]  ;push [idx]\n", (int)memory_indx);
        mov_xmm5_mem.size = strlen (mov_xmm5_mem.code);

        INIT_ASM_ELEM(push_to_stack);
        strcpy (push_to_stack.code, "vmovq qword [rsp], xmm5\n");
        push_to_stack.size = strlen (push_to_stack.code);

        INIT_ASM_ELEM(sub_rsp);
        strcpy (sub_rsp.code, "sub rsp, 8\n");
        sub_rsp.size = strlen (sub_rsp.code);

        WriteAsmCommand (dst_code, mov_xmm5_mem);
        WriteAsmCommand (dst_code, push_to_stack);
        WriteAsmCommand (dst_code, sub_rsp);

        SPACE(1);
}

inline void TranslatePushRAM (assembly_code* const dst_code, const u_int64_t memory_indx) // проверить корректность
{
        opcode get_mem = {};

        if (memory_indx <= 127)
        {
                get_mem.code = VMOVQ_XMM5_R13_B_IMM | memory_indx << BYTE(5);
                get_mem.size = OPSIZE(VMOVQ_XMM5_R13_B_IMM);
                WriteCommand (dst_code, get_mem);
        }
        else
        {
                opcode long_index =
                {
                        .code = memory_indx,
                        .size = DWORD
                };

                get_mem.code = VMOVQ_XMM5_R13_D_IMM;
                get_mem.size = OPSIZE(VMOVQ_XMM5_R13_D_IMM);

                WriteCommand (dst_code, get_mem);
                WriteCommand (dst_code, long_index);
        }


        constexpr opcode mov_to_stack =
        {
                .code = VMOVQ_RSP_XMM | XMM5_BASE << BYTE(3),                   // проверить корректность
                .size = OPSIZE(VMOVQ_RSP_XMM)
        };

        constexpr opcode sub_rsp =
        {
                .code = SUB_RSP_IMM | QWORD << BYTE(3),
                .size = OPSIZE(SUB_RSP_IMM)
        };

        WriteCommand (dst_code, mov_to_stack);
        WriteCommand (dst_code, sub_rsp);

}

u_int64_t cvt_host_reg_id_to_native (const int host_reg_id, const u_int64_t suffix, const u_int64_t offset)
{

        switch  (host_reg_id)
        {
        case RAX:
                return XMM1 << offset | suffix;
        case RBX:
                return XMM2 << offset | suffix;
        case RCX:
                return XMM3 << offset | suffix;
        case RDX:
                return XMM4 << offset | suffix;
        }
        return 0;
}

inline void TranslatePushREGToAsm (assembly_code* const dst_code, const u_int64_t reg_id)
{
        INIT_ASM_ELEM(mov_to_stack); // поменять на константу и спросить у криса так ли это делать
        sprintf (mov_to_stack.code, "movq qword [rsp - 16], xmm%d  ; push reg\n", (int)(reg_id - AX)); // проверить соответствие регистров
        mov_to_stack.size = strlen (mov_to_stack.code);

        INIT_ASM_ELEM(sub_rsp);
        sprintf (sub_rsp.code, "sub rsp, 16\n");
        sub_rsp.size = strlen (sub_rsp.code);

        WriteAsmCommand (dst_code, mov_to_stack);
        WriteAsmCommand (dst_code, sub_rsp);

        SPACE(1);
}

inline void TranslatePushREG (assembly_code* const dst_code, const u_int64_t reg_id)
{
        constexpr u_int64_t offset = 3;  // to binary move, to make correct mask
        constexpr u_int64_t suffix = 4;  // (4 = 00000100b)

        u_int64_t reg_op_code = cvt_host_reg_id_to_native(reg_id, suffix, offset);

        opcode vmovq_code =
        {
                .code = VMOVQ_RSP_XMM | reg_op_code << BYTE(3),                 // проверить
                .size = OPSIZE(VMOVQ_RSP_XMM)
        };

        WriteCommand (dst_code, vmovq_code);

        constexpr opcode rsp_sub_code =
        {
                .code = SUB_RSP_IMM | XMMWORD << BYTE(3),                       // проверить
                .size = OPSIZE(SUB_RSP_IMM)
        };

        WriteCommand (dst_code, rsp_sub_code);
}

inline void TranslatePopRAMToAsm (assembly_code* const dst_code, const u_int64_t memory_indx)
{
        INIT_ASM_ELEM(mov_to_xmm5);
        strcpy (mov_to_xmm5.code, "vmovq xmm5, qword [rsp + 8]  ; pop [idx]\n");
        mov_to_xmm5.size = strlen (mov_to_xmm5.code);

        INIT_ASM_ELEM(pop_ram);
        sprintf (mov_to_xmm5.code, "vmovq qword [r13 + 8 * %d], xmm5\n", (int)memory_indx);
        pop_ram.size = strlen (mov_to_xmm5.code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp.code, "add rsp, 8\n");
        add_rsp.size = strlen (add_rsp.code);

        WriteAsmCommand (dst_code, mov_to_xmm5);
        WriteAsmCommand (dst_code, pop_ram);
        WriteAsmCommand (dst_code, add_rsp);

        SPACE(1);
}

inline void TranslatePopRAM (assembly_code* const dst_code, const u_int64_t memory_indx)
{
        constexpr opcode add_rsp =
        {
                .code = ADD_RSP_IMM | XMMWORD << BYTE(3),
                .size = OPSIZE(ADD_RSP_IMM)
        };

        constexpr opcode get_top =
        {
                .code = VMOVQ_XMM_RSP_IMM | XMM5_EXTEND << BYTE(3) | XMMWORD << BYTE(5),
                .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
        };

        WriteCommand (dst_code, get_top);

        opcode pop_mem = {};

        if (memory_indx <= 127)
        {
                pop_mem.code = VMOVQ_R13_B_IMM_XMM5 | memory_indx << BYTE(5);
                pop_mem.size = OPSIZE(VMOVQ_R13_B_IMM_XMM5);
                WriteCommand (dst_code, pop_mem);
        }
        else
        {
                opcode memory_long_indx =
                {
                        .code = memory_indx,
                        .size = DWORD
                };

                pop_mem.code = VMOVQ_R13_D_IMM_XMM5;
                pop_mem.size = OPSIZE(VMOVQ_R13_D_IMM_XMM5);

                WriteCommand (dst_code, pop_mem);
                WriteCommand (dst_code, memory_long_indx);
        }

        WriteCommand (dst_code, add_rsp);
}

inline void TranslatePopREGToAsm (assembly_code* const dst_code, const int reg_id)
{
        INIT_ASM_ELEM(mov_xmm_stack);
        sprintf (mov_xmm_stack.code, "vmovq xmm%d, qword [rsp]  ; pop reg\n", (int)(reg_id - AX));// проверить соответствие регистров
        mov_xmm_stack.size = strlen (mov_xmm_stack.code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp.code, "add rsp, 16\n");
        add_rsp.size = strlen (add_rsp.code);

        WriteAsmCommand (dst_code, mov_xmm_stack);
        WriteAsmCommand (dst_code, add_rsp);

        SPACE(1);
}

inline void TranslatePopREG (assembly_code* const dst_code, const int reg_id)  // проверить все и протестировать
{
        constexpr u_int64_t suffix = 0x44; // 0x44 = 01000100b watch explanation of opcodes higher
        constexpr u_int64_t offset = 3;

        u_int64_t reg_op_code = cvt_host_reg_id_to_native(reg_id, suffix, offset);

        opcode movq_xmm_rsp =
        {
                .code = VMOVQ_XMM_RSP_IMM | reg_op_code << BYTE(3) | XMMWORD << BYTE(5),
                .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
        };

        WriteCommand (dst_code, movq_xmm_rsp);

        constexpr opcode rsp_sub_code =
        {
                .code = ADD_RSP_IMM | (XMMWORD << BYTE(3)),
                .size = OPSIZE(ADD_RSP_IMM)
        };

        WriteCommand (dst_code, rsp_sub_code);
}

// inline void TranslatePopToAsm (assembly_code* const dst_code)
// {
//         INIT_ASM_ELEM(rsp_sub_code);
//         strcpy (rsp_sub_code->code, "add rsp, 8\n");
//         rsp_sub_code->size = strlen (rsp_sub_code->code);
//
//         WriteAsmCommand (dst_code, rsp_sub_code);
//
//         SPACE(1);
// }
//
// inline void TranslatePop (assembly_code* const dst_code)
// {    // не верно так как у меня поп это изначально в регистр
//         constexpr opcode rsp_sub_code =
//         {
//                 .code = ADD_RSP_IMM | (XMMWORD << BYTE(3)),  // проверить xmm
//                 .size = OPSIZE(ADD_RSP_IMM)
//         };
//
//         WriteCommand (dst_code, rsp_sub_code);
// }

inline void TranslateArithmeticOpToAsm (assembly_code* const dst_code, const int op_id)
{
        INIT_ASM_ELEM(mov_xmm5_stack);
        strcpy (mov_xmm5_stack.code, "vmovq xmm5, qword [rsp + 16]  ; arithmetic\n");
        mov_xmm5_stack.size = strlen (mov_xmm5_stack.code);

        char operation_code[6] = {}; // заменить на константу

        switch (op_id)
        {
        case ADD:
                memcpy (operation_code, "addsd", 6);  // проверить
                break;
        case SUB:
                memcpy (operation_code, "subsd", 6);
                break;
        case MUL:
                memcpy (operation_code, "mulsd", 6);
                break;
        case DIV:
                memcpy (operation_code, "divsd", 6);
                break;
        }

        INIT_ASM_ELEM(arithm_op);
        sprintf (arithm_op.code, "%s xmm5, qword [rsp]\n", operation_code);
        arithm_op.size = strlen (arithm_op.code);

        INIT_ASM_ELEM(push_val);
        strcpy (push_val.code, "movq qword [rsp + 16], xmm5\n");
        push_val.size = strlen (push_val.code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp.code, "add rsp, 16\n");
        add_rsp.size = strlen (add_rsp.code);

        WriteAsmCommand (dst_code, mov_xmm5_stack);
        WriteAsmCommand (dst_code, arithm_op);
        WriteAsmCommand (dst_code, push_val);
        WriteAsmCommand (dst_code, add_rsp);

        SPACE(1);
}

inline void TranslateArithmeticOp (assembly_code* const dst_code, const int op_id)
{
        u_int64_t operation_code = 0;

        switch (op_id)
        {
        case ADD:
                operation_code = ADDSD_XMM_RSP_IMM;
                break;
        case SUB:
                operation_code = SUBSD_XMM_RSP_IMM;
                break;
        case MUL:
                operation_code = MULSD_XMM_RSP_IMM;
                break;
        case DIV:
                operation_code = DIVSD_XMM_RSP_IMM;
                break;
        }

        constexpr opcode get_operand =
        {
                .code = VMOVQ_XMM_RSP_IMM | XMM5_EXTEND << BYTE(3) | XMMWORD << BYTE(5),  // осмотреть на размер xmm
                .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
        };

        WriteCommand (dst_code, get_operand);

        opcode arithmetic_op =
        {
                .code =  operation_code | XMM5_EXTEND << BYTE(3) | 2 * XMMWORD << BYTE(5),
                .size =  OPSIZE(ADDSD_XMM_RSP_IMM)
        };

        WriteCommand (dst_code, arithmetic_op);

        constexpr opcode save_result =
        {
                .code = VMOVQ_RSP_IMM_XMM | XMM5_EXTEND << BYTE(3) | 2 * XMMWORD << BYTE(5),
                .size = OPSIZE(VMOVQ_RSP_IMM_XMM)
        };

        WriteCommand (dst_code, save_result);
}

inline void TranslateSqrtToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(mov_xmm0_stack);
        strcpy (mov_xmm0_stack.code, "vmovq xmm4, qword [rsp]  ; sqrt\n");
        mov_xmm0_stack.size = strlen (mov_xmm0_stack.code);

        INIT_ASM_ELEM(sqrt_xmm0);
        strcpy (sqrt_xmm0.code, "vsqrtpd xmm4, xmm4\n");
        sqrt_xmm0.size = strlen (sqrt_xmm0.code);

        INIT_ASM_ELEM(push_xmm0);
        strcpy (push_xmm0.code, "vmovq qword [rsp], xmm4\n");
        push_xmm0.size = strlen (push_xmm0.code);

        WriteAsmCommand (dst_code, mov_xmm0_stack);
        WriteAsmCommand (dst_code, sqrt_xmm0);
        WriteAsmCommand (dst_code, push_xmm0);

        SPACE(1);
}

inline void TranslateSqrt (assembly_code* const dst_code)
{
        constexpr opcode get_top =
        {
                .code = VMOVQ_XMM_RSP_IMM | XMM0_EXTEND << BYTE(3) | XMMWORD << BYTE(5),  // проверить
                .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
        };

        WriteCommand (dst_code, get_top);

        constexpr opcode vsqrt_xmm0 =
        {
                .code = VSQRTPD_XMM0_XMM0,
                .size = OPSIZE(VSQRTPD_XMM0_XMM0)
        };

        WriteCommand (dst_code, vsqrt_xmm0);

        constexpr opcode save_res =
        {
                .code = VMOVQ_RSP_IMM_XMM | XMM0_EXTEND << BYTE(3) | XMMWORD << BYTE(5),
                .size = OPSIZE(VMOVQ_RSP_IMM_XMM)
        };

        WriteCommand (dst_code, save_res);
}

inline void TranslateRetToAsm (assembly_code* const dst_code)
{
        // INIT_ASM_ELEM(alignment);
        // strcpy (alignment.code, "add rsp, 8  ; ret\n");
        // alignment.size = strlen (alignment.code);

        INIT_ASM_ELEM(ret);
        strcpy (ret.code, "ret\n");
        ret.size = strlen (ret.code);

        // WriteAsmCommand (dst_code, alignment);
        WriteAsmCommand (dst_code, ret);

        SPACE(1);
}

inline void TranslateRet (assembly_code* const dst_code)
{
        constexpr opcode add_rsp_alignment_size =
        {
                .code = ADD_RSP_IMM | XMMWORD << BYTE(3),  // посмотреть xmm
                .size = OPSIZE(ADD_RSP_IMM)
        };

        WriteCommand (dst_code, add_rsp_alignment_size);

        *dst_code->code = (char)RET_OP_CODE;
        dst_code->code++;
        dst_code->position++;
}

inline void TranslateLoadRsp (assembly_code* const dst_code)
{
        constexpr opcode mov_rsp_r15 =
        {
                .code = MOV_RSP_R15,
                .size = OPSIZE(MOV_RSP_R15)
        };
        WriteCommand (dst_code, mov_rsp_r15);
}

inline void TranslateHltToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(init_rsp);
        strcpy (init_rsp.code, "mov rsp, rbp  ; hlt\n");
        init_rsp.size = strlen (init_rsp.code);

        INIT_ASM_ELEM(ret);
        strcpy (ret.code, "jmp END\n");
        ret.size = strlen (ret.code);

        WriteAsmCommand (dst_code, init_rsp);
        WriteAsmCommand (dst_code, ret);

        SPACE(1);
}

inline void TranslateHlt (assembly_code* const dst_code)
{
        TranslateLoadRsp (dst_code);
        *dst_code->code = (char)RET_OP_CODE;
        dst_code->code++;
        dst_code->position++;
}

inline void TranslateInToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(mov_rdi_str);                             // аргумент функции
        strcpy (mov_rdi_str.code, "lea rdi, scanf_str  ; in\n");
        mov_rdi_str.size = strlen (mov_rdi_str.code);

        INIT_ASM_ELEM(sub_rsp);                             // аргумент функции
        strcpy (sub_rsp.code, "sub rsp, 16\n");
        sub_rsp.size = strlen (sub_rsp.code);

        INIT_ASM_ELEM(mov_rdi_rsi);                             // аргумент функции
        strcpy (mov_rdi_rsi.code, "mov rsi, rsp\n");
        mov_rdi_rsi.size = strlen (mov_rdi_rsi.code);

        INIT_ASM_ELEM(call_scanf);
        strcpy (call_scanf.code, "call scanf\n");        // функция из C
        call_scanf.size = strlen (call_scanf.code);

        WriteAsmCommand (dst_code, mov_rdi_str);
        WriteAsmCommand (dst_code, sub_rsp);
        WriteAsmCommand (dst_code, mov_rdi_rsi);
        WriteAsmCommand (dst_code, call_scanf);

        SPACE(1);
}

inline void TranslateIn (assembly_code* const dst_code)
{
        constexpr opcode mov_rdi_rsp =
        {
                .code = MOV_RDI_RSP,
                .size = OPSIZE(MOV_RDI_RSP)
        };

        constexpr opcode sub_rsp_xmmword =
        {
                .code = SUB_RSP_IMM | XMMWORD << BYTE(3), // протестировать
                .size = OPSIZE(SUB_RSP_IMM)
        };

        WriteCommand (dst_code, mov_rdi_rsp);

        u_int64_t code_address = (u_int64_t)(dst_code->code + OPSIZE(CALL_OP_CODE));
        u_int64_t func_address = (u_int64_t)(float_scanf);

        int rel_address  = (int)(func_address - code_address);

        cvt_u_int64_t_int convert =
        {
                .rel_addr = (size_t)rel_address
        };

        opcode call_in =
        {
                .code = CALL_OP_CODE | convert.extended_address << BYTE(1),
                .size = OPSIZE(CALL_OP_CODE)
        };

        WriteCommand (dst_code, call_in);
        WriteCommand (dst_code, sub_rsp_xmmword);
}

#define PUSH_ALL_REGS                           \
do                                              \
{                                               \
        TranslatePushREG (dst_code, AX);        \
        TranslatePushREG (dst_code, BX);        \
        TranslatePushREG (dst_code, CX);        \
        TranslatePushREG (dst_code, DX);        \
}                                               \
while (0)


#define POP_ALL_REGS                            \
do {                                            \
        TranslatePopREG (dst_code, AX);         \
        TranslatePopREG (dst_code, BX);         \
        TranslatePopREG (dst_code, CX);         \
        TranslatePopREG (dst_code, DX);         \
}                                               \
while (0)

inline void TranslateOutToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(mov_rdi_str);                             // аргумент функции
        strcpy (mov_rdi_str.code, "lea rdi, printf_str  ; out\n");
        mov_rdi_str.size = strlen (mov_rdi_str.code);

        INIT_ASM_ELEM(xmm_count);                            // аргумент для функции
        strcpy (xmm_count.code, "mov rax, 1\n");
        xmm_count.size = strlen (xmm_count.code);

        INIT_ASM_ELEM(save_xmm0);                            // аргумент для функции
        strcpy (save_xmm0.code, "vmovq qword [rsp - 8], xmm0\nsub rsp, 8\n");
        save_xmm0.size = strlen (save_xmm0.code);

        INIT_ASM_ELEM(mov_xmm0_val);                            // аргумент для функции
        strcpy (mov_xmm0_val.code, "movq xmm0, qword [rsp + 8]\n");
        mov_xmm0_val.size = strlen (mov_xmm0_val.code);

        INIT_ASM_ELEM(push_all_regs);
        strcpy (push_all_regs.code,  "vmovq qword [rsp - 8], xmm1\nsub rsp, 8\nvmovq qword [rsp - 8], xmm2\nsub rsp, 8\nvmovq qword [rsp - 8], xmm3\nsub rsp, 8\nvmovq qword [rsp - 8], xmm4\nsub rsp, 8\n\n");
        push_all_regs.size = strlen (push_all_regs.code);

        INIT_ASM_ELEM(aligned_sub_8);
        strcpy (aligned_sub_8.code, "sub rsp, 8\n");      // функция из C
        aligned_sub_8.size = strlen (aligned_sub_8.code);

        INIT_ASM_ELEM(call_printf);
        strcpy (call_printf.code, "call printf\n");      // функция из C
        call_printf.size = strlen (call_printf.code);

        INIT_ASM_ELEM(aligned_add_8);
        strcpy (aligned_add_8.code, "add rsp, 8\n");      // функция из C
        aligned_add_8.size = strlen (aligned_add_8.code);

        INIT_ASM_ELEM(aligned_rsp);
        strcpy (aligned_rsp.code, "add rsp, 16\n");      // функция из C
        aligned_rsp.size = strlen (aligned_rsp.code);

        INIT_ASM_ELEM(pop_all_regs);
        strcpy (pop_all_regs.code, "\nvmovq xmm4, qword [rsp]\nadd rsp, 8\nvmovq xmm3, qword [rsp]\nadd rsp, 8\nvmovq xmm2, qword [rsp]\nadd rsp, 8\nvmovq xmm1, qword [rsp]\nadd rsp, 8\nvmovq xmm0, qword [rsp]\nadd rsp, 8\n");
        pop_all_regs.size = strlen (pop_all_regs.code);

        WriteAsmCommand (dst_code, mov_rdi_str);
        WriteAsmCommand (dst_code, xmm_count);
        WriteAsmCommand (dst_code, save_xmm0);
        WriteAsmCommand (dst_code, mov_xmm0_val);
        WriteAsmCommand (dst_code, push_all_regs);
        WriteAsmCommand (dst_code, aligned_sub_8);
        WriteAsmCommand (dst_code, call_printf);
        WriteAsmCommand (dst_code, aligned_add_8);
        WriteAsmCommand (dst_code, pop_all_regs);
        WriteAsmCommand (dst_code, aligned_rsp);

        SPACE(1);
}

inline void TranslateOut (assembly_code* const dst_code)
{
        opcode lea_rdi_rsp_16 =
        {
                .code = LEA_RDI_RSP_16,
                .size = OPSIZE(LEA_RDI_RSP_16)
        };

        WriteCommand (dst_code, lea_rdi_rsp_16);

        PUSH_ALL_REGS;

        u_int64_t code_address = (u_int64_t)(dst_code->code + OPSIZE(CALL_OP_CODE));
        u_int64_t func_address = (u_int64_t)(float_printf);

        int rel_address = (int)(func_address - code_address);

        cvt_u_int64_t_int convert =
        {
                .rel_addr = (size_t)rel_address
        };

        opcode call_out =
        {
                .code = CALL_OP_CODE | convert.extended_address << BYTE(1),
                .size = OPSIZE(CALL_OP_CODE)
        };

        WriteCommand (dst_code, call_out);

        POP_ALL_REGS;
}

#define CHECK_JMP (current_code & 0xFF)
#define GEN_JMP(jmp_type, cmp_type)                                                    \
        vcmppd.code = VCMPPD_XMM5_XMM0_XMM5 | jmp_type << BYTE(4);                     \
        vcmppd.size = OPSIZE(VCMPPD_XMM5_XMM0_XMM5);                                   \
        WriteCommand (dst_code, vcmppd);                                               \
        WriteCommand (dst_code, movmsk);                                               \
        constexpr opcode cmp                                                           \
        {                                                                              \
                .code = cmp_type,                                                      \
                .size = OPSIZE(cmp_type)                                               \
        };                                                                             \
        WriteCommand (dst_code, cmp);                                                  \
        jmp.code = JE_REL32;                                                           \
        jmp.size = OPSIZE(JE_REL32);

#define TRANS_JMP(name, ...)                                                           \
        INIT_ASM_ELEM(translate_call);                                                 \
        sprintf (name.code, __VA_ARGS__);                                              \
        name.size = strlen (name.code);                                                \
                                                                                       \
        WriteAsmCommand (dst_code, name);                                              \
                                                                                       \
        SPACE(1)


inline void TranslateJmpCallToAsm (assembly_code* const dst_code, const int jmp_type, int* code)
{ // передаем адрес на котором стоит команда
        switch (jmp_type)
        {
        case CALL:
        {
                TRANS_JMP(translate_call, "sub rsp, 8\ncall L%g  ; call\nadd rsp, 8\n", *((float*)(code + 1)));
                break;
        }
        case JMP:
        {
                TRANS_JMP(translate_call, "jmp L%g  ; jmp\n",  *(float*)(code + 1));
                break;
        }
        case JB:
        {
                TRANS_JMP(translate_call,  "vmovq xmm4, qword [rsp]  ; jb\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x11\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x1\n"
                                           "je L%g\n",                           *(float*)(code + 1));
                break;
        }
        case JA:
        {
                TRANS_JMP(translate_call, "vmovq xmm4, qword [rsp]  ; ja\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x1E\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x1\n"
                                           "je L%g\n",                           *(float*)(code + 1));
                break;
        }
        case JE:
        {
                TRANS_JMP(translate_call, "vmovq xmm4, qword [rsp]  ; je\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x0\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x3\n"
                                           "je L%g\n",                           *(float*)(code + 1));
                break;
        }
        case JNE:
        {
                TRANS_JMP(translate_call, "vmovq xmm4, qword [rsp]  ; jne\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x0\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x3\n"
                                           "jne L%g\n",                          *(float*)(code + 1));
                break;
        }
        case JAE:
        {
                TRANS_JMP(translate_call, "vmovq xmm4, qword [rsp]  ; jae\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "vmovq xmm4, xmm5\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x0\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x3\n"
                                           "je L%g\n"
                                           "vcmppd xmm4, xmm4, xmm4, 0x1E\n"
                                           "xor r14d, r14d\n"
                                           "movmskpd r14d, xmm4\n"
                                           "cmp r14d, 0x1\n"
                                           "je L%g\n",                           *(float*)(code + 1), *(float*)(code + 1));
                break;
        }
        case JBE:
        {
                TRANS_JMP(translate_call, "vmovq xmm4, qword [rsp]  ; jbe\n"
                                           "vmovq xmm5, qword [rsp + 16]\n"
                                           "vmovq xmm4, xmm5\n"
                                           "add rsp, 32\n\n"
                                           "vcmppd xmm5, xmm4, xmm5, 0x0\n"
                                           "movmskpd r14d, xmm5\n"
                                           "cmp r14d, 0x3\n"
                                           "je L%g\n"
                                           "vcmppd xmm4, xmm4, xmm4, 0x11\n"
                                           "xor r14d, r14d\n"
                                           "movmskpd r14d, xmm4\n"
                                           "cmp r14d, 0x1\n"
                                           "je L%g\n",                           *(float*)(code + 1), *(float*)(code + 1));
                break;
        }
        }
}

void TranslateRel32Label (assembly_code* const dst_code, const size_t jmp_pos)
{
        char* temp_code = dst_code->code;
        size_t to_sub   = dst_code->position - jmp_pos;
        dst_code->code -= to_sub;                               // получили адрес прыжка

        const u_int64_t current_code = *(uint_fast64_t*)dst_code->code;
        cvt_u_int64_t_int convert    = {};
        opcode write_rel32           = {};

        if (JMP_REL32 == CHECK_JMP || CALL_OP_CODE == CHECK_JMP)
        {
                convert.rel_addr = to_sub - OPSIZE(JMP_REL32);

                write_rel32.code = current_code | convert.extended_address << BYTE(1); // пишем jmp со сдивигом условно
                write_rel32.size = sizeof (u_int64_t);
        }
        else
        {
                convert.rel_addr = to_sub - OPSIZE(JE_REL32);

                write_rel32.code = current_code | convert.extended_address << BYTE(2);
                write_rel32.size = sizeof(u_int64_t);
        }

        WriteCommand (dst_code, write_rel32);

        dst_code->code     = temp_code;
        dst_code->position = jmp_pos + to_sub; // вернули начальные данные
}

opcode TranslateJmpCall (assembly_code* const dst_code, const int jmp_call_code)
{
        constexpr opcode movmsk =
        {
                .code = MOVMSKPD_R14D_XMM5,
                .size = OPSIZE(MOVMSKPD_R14D_XMM5)
        };

        opcode vcmppd = {};
        opcode jmp    = {};

        switch (jmp_call_code)
        {
        case JB:
        {
                GEN_JMP(LESS, CMP_R14D_1);
                break;
        }
        case JA:
        {
                GEN_JMP(GREATER, CMP_R14D_1);
                break;
        }
        case JE:
        {
                GEN_JMP(EQUAL, CMP_R14D_3);
                break;
        }
        case CALL:
        {
                TranslatePush (dst_code, (u_int64_t)dst_code->code + TRANSLATE_PUSH_SIZE);
                __attribute__((fallthrough));
        }
        case JMP:
        {
                jmp.code = JMP_REL32;
                jmp.size = OPSIZE(JMP_REL32);
                break;
        }
        }

        return jmp;
}

inline void TranslateTwoPopForCmp (assembly_code* const dst_code, const int jmp_call_code)
{
        if (jmp_call_code != JMP)
        {
                constexpr opcode get_first_elem =
                {
                        .code =  VMOVQ_XMM_RSP_IMM | XMM0_EXTEND << BYTE(3) | XMMWORD << BYTE(5),
                        .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
                };

                constexpr opcode get_second_elem =
                {
                        .code =  VMOVQ_XMM_RSP_IMM | XMM5_EXTEND << BYTE(3) | 2 * XMMWORD << BYTE(5),
                        .size = OPSIZE(VMOVQ_XMM_RSP_IMM)
                };

                constexpr opcode add_rsp =
                {
                        .code = ADD_RSP_IMM | 2 * XMMWORD << BYTE(3),
                        .size = OPSIZE(ADD_RSP_IMM)
                };

                WriteCommand (dst_code, get_first_elem);
                WriteCommand (dst_code, get_second_elem);
                WriteCommand (dst_code, add_rsp);
        }
}

inline void TranslateAheadJmpCall (assembly_code* const dst_code,
                                   label_table*   const table,
                                   const int            label_pos,
                                   const int            jmp_call_pos,
                                   const int            jmp_call_code)
{
        TranslateTwoPopForCmp (dst_code, jmp_call_code);

        opcode jmp = TranslateJmpCall (dst_code, jmp_call_code);

        size_t* code_pos_ptr = GetCodePosPtrByJmp (table, label_pos, jmp_call_pos);
        *code_pos_ptr        = dst_code->position;

        WriteCommand (dst_code, jmp);
}

inline void JmpCallHandler (assembly_code* const dst_code,
                            label_table*   const table,
                            const int      label_pos,
                            const int      jmp_call_pos,
                            const int      jmp_call_code)
{
        if (label_pos <= jmp_call_pos)
        {
                TranslateCycle (dst_code,
                                table,
                                label_pos,
                                jmp_call_pos,
                                jmp_call_code);
        }
        else
        {
                TranslateAheadJmpCall (dst_code,
                                       table,
                                       label_pos,
                                       jmp_call_pos,
                                       jmp_call_code);
        }
}

inline void TranslateCycle (assembly_code* const dst_code,
                            label_table*   const table,
                            const int            label_pos,
                            const int            jmp_call_pos,
                            const int            jmp_call_code)

{  // понять позже
        TranslateTwoPopForCmp (dst_code, jmp_call_code);

        size_t code_pos = GetCodePosByJmp (table, label_pos, jmp_call_pos);

        cvt_u_int64_t_int convert = {};

        opcode jmp = TranslateJmpCall (dst_code, jmp_call_code);

        switch(jmp_call_code)
        {
        case CALL:
                TranslatePush (dst_code, (u_int64_t)dst_code->code + TRANSLATE_PUSH_SIZE);
                __attribute__((fallthrough));
        case JMP:
                convert = { .rel_addr = code_pos - dst_code->position - OPSIZE(JMP_REL32)};
                jmp.code |= convert.extended_address << BYTE(1);
                break;
        default:
                convert = { .rel_addr = code_pos - dst_code->position - OPSIZE(JE_REL32)};
                jmp.code |= convert.extended_address << BYTE(2);
        }

        WriteCommand (dst_code, jmp);
}

inline void SetDataSegment (assembly_code* const dst_code)  // положили в первые ячейки сегмент памяти
{
        const u_int64_t data_address = (u_int64_t)(dst_code->code - 2 * PAGESIZE);

        constexpr opcode mov_r13_imm =
        {
                .code = MOV_R13,
                .size = OPSIZE(MOV_R13)
        };

        opcode write_data =
        {
                .code = data_address,
                .size = sizeof(u_int64_t)
        };

        WriteCommand (dst_code, mov_r13_imm);
        WriteCommand (dst_code, write_data);

}

inline void TranslateSaveRsp (assembly_code* const dst_code)
{
        constexpr opcode mov_r15_rsp =
        {
                .code = MOV_R15_RSP,
                .size = OPSIZE(MOV_R15_RSP)
        };

        WriteCommand (dst_code, mov_r15_rsp);
        constexpr u_int64_t qword_size = 0x08;

        constexpr opcode rsp_sub_code =
        {
                .code = SUB_RSP_IMM | (qword_size << BYTE(3)),
                .size = OPSIZE(SUB_RSP_IMM)
        };

        WriteCommand (dst_code, rsp_sub_code);
}

#define PUSH_HANDLER                                                                            \
        PUSHRAM:                                                                                \
        {                                                                                       \
        TranslatePushRAM (dst_buffer, (u_int64_t)code[iter_count + 1]);                         \
        if (mode)                                                                               \
                TranslatePushRAMToAsm (asm_dst_buffer, (u_int64_t)code[iter_count + 1]);            \
        iter_count++;                                                                           \
        break;                                                                                  \
        }                                                                                       \
        case PUSHREG:                                                                           \
        {                                                                                       \
        TranslatePushREG (dst_buffer, (u_int64_t)code[iter_count + 1]);                         \
        if (mode)                                                                               \
                TranslatePushREGToAsm (asm_dst_buffer, (u_int64_t)code[iter_count + 1]);            \
        iter_count++;                                                                           \
        break;                                                                                  \
        }                                                                                       \
        case PUSH:                                                                              \
        {                                                                                       \
        TranslatePush (dst_buffer, *(u_int64_t*)&code[iter_count + 1]);                         \
        if (mode)                                                                               \
                TranslatePushToAsm (asm_dst_buffer, code[iter_count + 1]);            \
        iter_count++;                                                                           \
        break;                                                                                  \
        }

#define POP_HANDLER                                                                             \
        POP:                                                                                    \
        {                                                                                       \
        TranslatePopREG (dst_buffer, code[iter_count + 1]);                                     \
        if (mode)                                                                               \
                TranslatePopREGToAsm (asm_dst_buffer, code[iter_count + 1]);                        \
        iter_count++;                                                                           \
        break;                                                                                  \
        }                                                                                       \
        case POPRAM:                                                                            \
        {                                                                                       \
        TranslatePopRAM (dst_buffer, (u_int64_t)code[iter_count + 1]);                          \
        if (mode)                                                                               \
                TranslatePopREGToAsm (asm_dst_buffer, (u_int64_t)code[iter_count + 1]);             \
        iter_count++;                                                                           \
        break;                                                                                  \
        }

#define CALL_JUMP_HANDLER                                                                       \
        CALL:                                                                                   \
        case JB:                                                                                \
        case JE:                                                                                \
        case JA:                                                                                \
        case JMP:                                                                               \
        {                                                                                       \
        JmpCallHandler (dst_buffer, &table, code[iter_count + 1],                               \
                        iter_count, code[iter_count]);                                          \
        if (mode)                                                                               \
                TranslateJmpCallToAsm (asm_dst_buffer, code[iter_count], (int*)(&(code[iter_count])));      \
        iter_count++;                                                                           \
        break;                                                                                  \
        }

#define ARITHMETIC_OPERATIONS                                                                   \
        ADD:                                                                                    \
        case DIV:                                                                               \
        case MUL:                                                                               \
        case SUB:                                                                               \
        {                                                                                       \
        TranslateArithmeticOp (dst_buffer, code[iter_count]);                                   \
        if (mode)                                                                               \
                TranslateArithmeticOpToAsm (asm_dst_buffer, code[iter_count]);                      \
        break;                                                                                  \
        }                                                                                       \
        case SQRT:                                                                              \
        {                                                                                       \
        TranslateSqrt (dst_buffer);                                                             \
        if (mode)                                                                               \
                TranslateSqrtToAsm (asm_dst_buffer);                                                \
        break;                                                                                  \
        }

#define STDIO_HANDLER                                                                           \
        IN:                                                                                     \
        {                                                                                       \
        TranslateIn (dst_buffer);                                                               \
        if (mode)                                                                               \
                TranslateInToAsm (asm_dst_buffer);                                                  \
        break;                                                                                  \
        }                                                                                       \
        case OUT:                                                                               \
        {                                                                                       \
        TranslateOut (dst_buffer);                                                              \
        if (mode)                                                                               \
                TranslateOutToAsm (asm_dst_buffer);                                                 \
        break;                                                                                  \
        }

#define RET_HLT_HANLDER                                                                         \
        RET:                                                                                    \
        {                                                                                       \
        TranslateRet (dst_buffer);                                                              \
        if (mode)                                                                               \
                TranslateRetToAsm (asm_dst_buffer);                                                 \
        break;                                                                                  \
        }                                                                                       \
        case HLT:                                                                               \
        {                                                                                       \
        TranslateHlt (dst_buffer);                                                              \
        if (mode)                                                                               \
                TranslateHltToAsm (asm_dst_buffer);                                                 \
        break;                                                                                  \
        }

#define LABEL_SET                                                                               \
        search_res = LabelTableSearchByLabel (&table, iter_count);                              \
                                                                                                \
        if (search_res != NOT_FOUND)                                                            \
                LinkLabel (dst_buffer,                                                          \
                           &table,                                                              \
                           search_res,                                                          \
                           iter_count);

#define CODE_POS(count)   table->elems[search_idx].data[count].code_pos
#define JMP_POS(count)    table->elems[search_idx].data[count].jmp
#define LABEL_POS(count)  table->elems[search_idx].data[count].label

void LinkLabel (assembly_code* const dst_code, label_table* const table, const int search_idx, const int label_pos)
{
        for (int iter_count = 0; iter_count < table->elems[search_idx].size; iter_count++)
        {
                if (label_pos == LABEL_POS(iter_count))
                {
                        if (JMP_POS(iter_count) >= LABEL_POS(iter_count))
                                CODE_POS(iter_count) = dst_code->position;
                        else
                                TranslateRel32Label (dst_code, CODE_POS(iter_count));
                }
        }
}

inline void SetLabel (assembly_code* const dst_code, const int label_pos)
{
        INIT_ASM_ELEM(label);
        sprintf (label.code, "L%d:\n", label_pos);
        label.size = strlen (label.code);
        WriteAsmCommand (dst_code, label);
}

void CheckNSetLabel (assembly_code* const dst_code, int* jmp_pos, const size_t iter_count) // не рассмотрен частный случай где метка равна 0
{
        for (size_t i = 0; jmp_pos[i] != 0; i++)
        {
                if (jmp_pos[i] == iter_count)
                {
                        SetLabel (dst_code, iter_count);
                }
        }
}

void TranslationStart (const char* const src_file_name, assembly_code* const dst_buffer, assembly_code* const asm_dst_buffer, const int mode)
{
        assembly_code src_code = {};
        LoadBinaryCode (src_file_name, &src_code);
        float* code = (float*)src_code.code;

        asm_dst_buffer->code = (char*)calloc (MIN_DST_CODE_SIZE, sizeof (char));

        AssemblyCodeAlignedInit (dst_buffer, PAGESIZE, MIN_DST_CODE_SIZE);

        size_t op_count = src_code.size / sizeof(float);
        dst_buffer->code += 2 * PAGESIZE;
        SetDataSegment   (dst_buffer); // для бинарного кода
        TranslateSaveRsp (dst_buffer);

        label_table table = {};
        LabelTableInit (&table);
        MakeLabelTable (&src_code, &table); // для бинарного

        int* jmp_pos = nullptr;
        if (mode)
        {
                jmp_pos = MakeLabelTableToAsm (&src_code); // преобразовать
                // for (size_t i = 0; i < MIN_LABEL_TABLE_SIZE; i++)
                //         printf ("pos = %d\n", jmp_pos[i]);
        }

        int search_res = 0;

        WriteHeader (asm_dst_buffer);
        for (size_t iter_count = 0; iter_count < op_count; iter_count++)
        {
                if (mode)
                        CheckNSetLabel (asm_dst_buffer, jmp_pos, iter_count);
                LABEL_SET;

                switch ((int)code[iter_count])
                {
                case RET_HLT_HANLDER
                case CALL_JUMP_HANDLER

                case POP_HANDLER
                case PUSH_HANDLER

                case ARITHMETIC_OPERATIONS

                case STDIO_HANDLER
                }
        }
        WriteEnd (asm_dst_buffer);

        TranslateLoadRsp (dst_buffer);
        *dst_buffer->code = (char)RET_OP_CODE;
        dst_buffer->code -= dst_buffer->position;

        asm_dst_buffer->code -= asm_dst_buffer->position;
        if (mode)
        {
                printf ("%s\n", asm_dst_buffer->code);
        }

        FILE* dst_file = fopen ("dst_code.asm", "w");
        fwrite (asm_dst_buffer->code, sizeof (char), asm_dst_buffer->size, dst_file);
        fclose (dst_file);
}

int* MakeLabelTableToAsm (assembly_code* const src_code) // объединить со стандартным
{
        size_t    op_count = src_code->size / sizeof(float);
        float*        code = (float*)src_code->code;
        int*       jmp_pos = (int*)calloc (MIN_LABEL_TABLE_SIZE, sizeof (int));
        int* jmp_pos_start = jmp_pos;

        printf ("op_count = %ld\n", op_count);

        for (int iter_count = 0; iter_count < op_count; iter_count++)
        {
                switch ((int)code[iter_count])
                {
                case PUSH:
                        iter_count ++;
                        break;
                case CALL:
                case JB:
                case JE:
                case JA:
                case JAE:
                case JBE:
                case JNE:
                case JMP:
                        iter_count ++;
                        *jmp_pos = (int)code[iter_count];
                        jmp_pos ++;
                        break;
                case POP:
                case PUSHREG:
                case POPRAMREG:
                case PUSHRAM:
                case POPRAM:
                        iter_count ++;
                        break;
                case HLT:
                case RET:
                case OUT:
                case ADD:
                case DIV:
                case MUL:
                case SUB:
                case SQRT:
                case IN:
                        break;
                }
        }
        return jmp_pos_start;
}

void MakeLabelTable (assembly_code* const src_code, label_table* const table)
{// не проверено нормально, пока заняться другим
        size_t op_count = src_code->size / sizeof(float);
        float*       code = (float*)src_code->code;

        for (size_t iter_count = 0; iter_count < op_count; iter_count++)
        {
                switch ((int)code[iter_count])
                {
                case PUSH:
                        iter_count ++;   // проверить при тестировании
                        break;
                case CALL:
                case JB:
                case JE:
                case JA:
                case JAE:
                case JBE:
                case JNE:
                case JMP:
                        LabelTableAdd (table, code[iter_count + 1], iter_count);
                        iter_count++;
                        break;
                case POP:
                case PUSHREG:
                case POPRAMREG:
                case PUSHRAM:
                case POPRAM:
                        iter_count ++;
                        break;
                case HLT:
                case RET:
                case OUT:
                case ADD:
                case DIV:
                case MUL:
                case SUB:
                case SQRT:
                case IN:
                        break;
                }
        }
}

#define NON_NATIVE 1
#define TIME       2
#define ASM        3

#define SET_BIT(position) *option_mask |= (1 << position)
#define GET_BIT(position) option_mask  &  (1 << position)

void Handler (int argc, char* argv[])
{
        int option_mask     = 0;
        int file_name_index = 0;

        for (int iter_count = 1; iter_count < argc; iter_count++)
        {
                if (*argv[iter_count] == '-')
                        OptionDetecting (argv[iter_count], &option_mask);
                else
                {
                        if (file_name_index != 0)
                        {
                                printf("Fatal error: "
                                       "You cannot translate more "
                                       "than one file\n");
                                return;
                        }

                        file_name_index = iter_count;
                }
        }

        if (file_name_index == 0)
        {
                printf("Fatal error: "
                       "expected file name\n");
                return;
        }

        OptionHandling (option_mask, argv[file_name_index]);
}

inline void OptionDetecting (const char* const option, int* const option_mask) // поставили маску
{
        if (strcmp(option, "--non-native") == 0)
                SET_BIT(NON_NATIVE);
        else if (strcmp(option, "--time") == 0)
                SET_BIT(TIME);
        else if (strcmp(option, "--S") == 0)
                SET_BIT(ASM);
        else
                printf("Warning:"
                       "Unrecognizable option %s "
                       "was ignored.  Type --help "
                       "to show all options\n", option);
}

void  OptionHandling (int option_mask, const char* const file_name)
{
        if (GET_BIT(ASM))
        {
                assembly_code execute = {};
                assembly_code asm_execute = {};
                TranslationStart (file_name, &execute, &asm_execute, GET_BIT(ASM));
                ExecuteStart (execute.code);
        }
        else
        {
                assembly_code execute = {};
                assembly_code asm_execute = {};
                TranslationStart (file_name, &execute, &asm_execute, GET_BIT(ASM)); // изменить
                ExecuteStart (execute.code);
        }
}

void ExecuteStart (char* const execution_buffer)
{
        if (mprotect(execution_buffer, MIN_DST_CODE_SIZE, PROT_EXEC) != 0)
        {
                printf ("mprotect error\n");
                return;
        }
        void (*ExecutionAddress)(void) = (void (*)(void))(execution_buffer);
        ExecutionAddress();
}
