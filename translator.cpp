#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int LoadBinaryCode (const char* const src_file_name, assembly_code* const src_code_save)
{
        FILE* src_file = fopen (src_file_name, "rb");

        fseek (src_file, 0, SEEK_END);
        int file_size = ftell (src_file);
        fseek (src_file, 0, SEEK_SET);

        fread (src_code_save->code, sizeof (char), file_size, src_file);

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
        memcpy (dst_code->code, assembler_code->code, assembler_code.size);
        dst_code->position += assembler_code.size;
        dst_code->code     += assembler_code.size;
}

inline void TranslatePushToAsm (assembly_code* const dst_code, const u_int64_t data)
{
        INIT_ASM_ELEM(mov_r14_imm); // поменять на константу и спросить у криса так ли это делать
        sprintf (mov_r14_imm->code, "movabs r14, %llu\n", data);
        mov_r14_imm->size = strlen (mov_r14_imm->code);

        INIT_ASM_ELEM(mov_and_redata);
        strcpy (mov_and_redata->code, "mov qword [rsp], r14\n
                                       sub rsp, 8\n");
        mov_and_redata->size = sizeof (mov_and_redata->code);

        WriteAsmCommand (dst_code, mov_r14_imm);
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
        sprintf (mov_xmm5_mem->code, "vmovq xmm5, qword [r13 + 8 * %d]\n", (int)memory_indx); // r13 - start mem addr
        mov_xmm5_mem->size = strlen (mov_xmm5_mem->code);

        INIT_ASM_ELEM(push_to_stack);
        strcpy (push_to_stack->code, "vmovq qword [rsp], xmm5\n");
        push_to_stack->size = strlen (push_to_stack->code);

        INIT_ASM_ELEM(sub_rsp);
        strcpy (sub_rsp->code, "sub rsp, 8\n");
        sub_rsp->size = strlen (sub_rsp->code);

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
}

inline void TranslatePushREGToAsm (assembly_code* const dst_code, const u_int64_t reg_id)
{
        INIT_ASM_ELEM(mov_to_stack); // поменять на константу и спросить у криса так ли это делать
        sprintf (mov_to_stack->code, "movq qword [rsp], xmm%d\n", (int)(reg_id - AX)); // проверить соответствие регистров
        mov_to_stack->size = strlen (mov_to_stack->code);

        INIT_ASM_ELEM(mov_to_stack);
        asmcode sub_rsp =
        {
                .code = (char*)calloc (32, sizeof (char));
                .size = 0;
        };
        sprintf (sub_rsp->code, "sub rsp, 8\n");

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
{ // разобрать случай с большими индексами памяти, проверить выполняется ли
        INIT_ASM_ELEM(mov_to_xmm5);
        strcpy (mov_to_xmm5->code, "vmovq xmm5, qword [rsp + 8]\n");
        mov_to_xmm5->size = strlen (mov_to_xmm5->code);

        INIT_ASM_ELEM(pop_ram);
        sprintf (mov_to_xmm5->code, "vmovq qword [r13 + 8 * %d], xmm5\n", (int)memory_indx);
        pop_ram->size = strlen (mov_to_xmm5->code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp->code, "add rsp, 8\n");
        add_rsp->size = strlen (add_rsp->code);

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
        sprintf (mov_xmm_stack->code, "vmovq xmm%d, qword [rsp + 8]\n", (int)(reg_id - AX));// проверить соответствие регистров
        mov_xmm_stack->size = strlen (mov_xmm_stack->code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp->code, "add rsp, 8\n");
        add_rsp->size = strlen (add_rsp->code);

        WriteAsmCommand (dst_code, mov_xmm_stack);
        WriteAsmCommand (dst_code, add_rsp);

        SPACE(1);
}

inline void TranslatePopREG (assembly_code* const dst_code, const int reg_id)  // проверить все и протестировать
{  // как pop
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

inline void TranslatePopToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(rsp_sub_code);
        strcpy (rsp_sub_code->code, "add rsp, 8\n");
        rsp_sub_code->size = strlen (rsp_sub_code->code);

        WriteAsmCommand (dst_code, rsp_sub_code);

        SPACE(1);
}

inline void TranslatePop (assembly_code* const dst_code)
{    // не верно так как у меня поп это изначально в регистр
        constexpr opcode rsp_sub_code =
        {
                .code = ADD_RSP_IMM | (XMMWORD << BYTE(3)),  // проверить xmm
                .size = OPSIZE(ADD_RSP_IMM)
        };

        WriteCommand (dst_code, rsp_sub_code);
}

inline void TranslateArithmeticOpToAsm (assembly_code* const dst_code, const int op_id)
{
        INIT_ASM_ELEM(mov_xmm5_stack);
        strcpy (mov_xmm5_stack->code, "vmovq xmm5, qword [rsp+8]\n");
        mov_xmm5_stack->size = strlen (mov_xmm5_stack->code);

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
        sprintf (arithm_op->code, "%s xmm5, qword [rsp+16]\n", operation_code);
        arithm_op->size = strlen (arithm_op->code);

        INIT_ASM_ELEM(add_rsp);
        strcpy (add_rsp->code, "add rsp, 8\n");
        add_rsp->size = strlen (add_rsp->code);

        WriteAsmCommand (dst_code, mov_xmm5_stack);
        WriteAsmCommand (dst_code, arithm_op);
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
        strcpy (mov_xmm0_stack->code, "vmovq xmm0, qword [rsp+8]\n");
        mov_xmm0_stack->size = strlen (mov_xmm0_stack->code);

        INIT_ASM_ELEM(sqrt_xmm0);
        strcpy (sqrt_xmm0->code, "vsqrtpd xmm0, xmm0\n");
        sqrt_xmm0->size = strlen (sqrt_xmm0->code);

        INIT_ASM_ELEM(push_xmm0);
        strcpy (push_xmm0->code, "vmovq [rsp+8], xmm0\n");
        push_xmm0->size = strlen (push_xmm0->code);

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
        INIT_ASM_ELEM(alignment);
        strcpy (alignment->code, "add rsp, 8\n");
        alignment->size = strlen (alignment->code);

        INIT_ASM_ELEM(ret);
        strcpy (ret->code, "ret\n");
        ret->size = strlen (ret->code);

        WriteAsmCommand (dst_code, alignment);
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
        strcpy (init_rsp->code, "mov rsp, rbp\n");
        init_rsp->size = strlen (init_rsp->code);

        INIT_ASM_ELEM(ret);
        strcpy (ret->code, "ret\n");
        ret->size = strlen (ret->code);

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
{       // проверить куд возвращвется, по факту в stack
        INIT_ASM_ELEM(mov_rdi_rsi);                             // аргумент функции
        strcpy (mov_rdi_rsi->code, "mov rdi, rsp\n");
        mov_rdi_rsi->size = strlen (mov_rdi_rsi->code);

        INIT_ASM_ELEM(call_scanf);
        strcpy (call_scanf->code, "call float_scanf\n");        // функция из C
        call_scanf->size = strlen (call_scanf->code);

        INIT_ASM_ELEM(sub_rsp);                             // аргумент функции
        strcpy (sub_rsp->code, "sub rsp, 8\n");
        sub_rsp->size = strlen (sub_rsp->code);

        WriteAsmCommand (dst_code, mov_rdi_rsi);
        WriteAsmCommand (dst_code, call_scanf);
        WriteAsmCommand (dst_code, sub_rsp);

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
                .rel_addr = rel_address
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
        TranslatePopPEG (dst_code, AX);         \
        TranslatePopPEG (dst_code, BX);         \
        TranslatePopPEG (dst_code, CX);         \
        TranslatePopPEG (dst_code, DX);         \
}                                               \
while (0)

inline void TranslateOutToAsm (assembly_code* const dst_code)
{
        INIT_ASM_ELEM(lea_rdi_addr);                            // аргумент для функции
        strcpy (lea_rdi_addr->code, "lea rdi, [rsp + 8]\n");
        lea_rdi_addr->size = strlen (lea_rdi_addr->code);

        INIT_ASM_ELEM(push_all_regs);
        strcpy (push_all_regs->code, "vmovq qword [rsp], xmm1\n
                                      sub rsp, 8\n
                                      vmovq qword [rsp], xmm2\n
                                      sub rsp, 8\n
                                      vmovq qword [rsp], xmm3\n
                                      sub rsp, 8\n
                                      vmovq qword [rsp], xmm4\n
                                      sub rsp, 8\n");
        push_all_regs->size = strlen (push_all_regs->code);

        INIT_ASM_ELEM(call_printf);
        strcpy (call_printf->code, "call float_printf\n");      // функция из C
        call_printf->size = strlen (call_printf->code);

        INIT_ASM_ELEM(pop_all_regs);
        strcpy (pop_all_regs->code, "vmovq xmm4, qword [rsp+8]
                                     add rsp, 8
                                     vmovq xmm4, qword [rsp+8]
                                     add rsp, 8
                                     vmovq xmm4, qword [rsp+8]
                                     add rsp, 8
                                     vmovq xmm4, qword [rsp+8]
                                     add rsp, 8");
        pop_all_regs->size = strlen (pop_all_regs->code);

        WriteAsmCommand (dst_code, lea_rdi_addr);
        WriteAsmCommand (dst_code, push_all_regs);
        WriteAsmCommand (dst_code, call_printf);
        WriteAsmCommand (dst_code, pop_all_regs);

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

        SAVE_ALL_REGS;

        u_int64_t code_address = (u_int64_t)(dst_code->code + OPSIZE(CALL_OP_CODE));
        u_int64_t func_address = (u_int64_t)(float_printf);

        int rel_address = (int)(func_address - code_address);

        cvt_u_int64_t_int convert =
        {
                .rel_addr = rel_address
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

inline void TranslateAheadJmpCall (assembly_code* const dst_code,   // зависит от таблицы меток
                                   abel_table*    const table,
                                   const int            label_pos,
                                   const int            jmp_call_pos,
                                   const int            jmp_call_code)
{ // доделать часть с таблицей
        TranslateTwoPopForCmp (dst_code, jmp_call_code);

        opcode jmp = TranslateJmpCall (dst_code, jmp_call_code);

        size_t* code_pos_ptr = get_code_pos_ptr_by_jmp(table, label_pos, jmp_call_pos);
        // дописать либу с таблицей меток
        *code_pos_ptr        = dst_code->position;

        WriteCommand (dst_code, jmp);

}

inline void JmpCallHandler (assembly_code* const __restrict dst_code,         // нужно таблицу написать
                            label_table*   const __restrict table,
                            const int                       label_pos,
                            const int                       jmp_n_call_pos,
                            const int                       jmp_n_call_code)
{  // не смотрел
        // For case when label is before jump
        if (label_pos <= jmp_call_pos) {
                translate_cycle(dst_code,
                                table,
                                label_pos,
                                jmp_n_call_pos,
                                jmp_n_call_code);
        }
        else
        {
                translate_ahead_jmp_n_call(dst_code,
                                           table,
                                           label_pos,
                                           jmp_n_call_pos,
                                           jmp_n_call_code);
        }
                        // For case label after jump


}

inline void TranslateCycle (assembly_code* const dst_code,      // нужно таблицу написать
                            label_table*   const table,
                            const int            label_pos,
                            const int            jmp_call_pos,
                            const int            jmp_call_code)

{ // не смотрел
        TranslateTwoPopForCmp (dst_code, jmp_call_code);

        size_t code_pos = get_code_pos_by_jmp (table,
                                               label_pos,
                                               jmp_n_call_pos);

        cvt_u_int64_t_int convert = {};

        opcode jmp = translate_jmp_n_call(dst_code, jmp_n_call_code);

        // Call is treated as a jump with saving the return address in the stack
        switch(jmp_n_call_code) {
        case CALL:
                translate_push(dst_code,
                               (u_int64_t)dst_code->code + TRANSLATE_PUSH_SIZE);
                __attribute__((fallthrough));

        case JMP:
                // Save rel address
                convert = { .rel_addr =
                            code_pos - dst_code->position - OPSIZE(JMP_REL32)};
                jmp.code |= convert.extended_address << BYTE(1);
                break;
       default:
                convert = { .rel_addr = code_pos - dst_code->position - OPSIZE(JE_REL32)};
                jmp.code |= convert.extended_address << BYTE(2);
        }

        write_command(dst_code, jmp);
}

inline void SetDataSegment (assembly_code* const dst_code)
{
        // положили в первые ячейки сегмент памяти

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

#define PUSH_HANDLER                                                    \
        PUSHRAM:                                                        \
        TranslatePushRAM (dst_buffer, (u_int64_t)code[iter_count + 1]); \
        iter_count++;                                                   \
        break;                                                          \
        case PUSHREG:                                                   \
        TranslatePushREG (dst_buffer, code[iter_count + 1]);            \
        iter_count++;                                                   \
        break;                                                          \
        case PUSH:                                                      \
        TranslatePush (dst_buffer, *(u_int64_t*)&code[iter_count + 1]); \
        iter_count++;                                                   \
        break

#define POP_HANDLER                                                     \
        POP:                                                            \
        TranslatePop (dst_buffer);                                      \
        break;                                                          \
        case POPREG:                                                    \
        TranslatePopREG (dst_buffer, code[iter_count + 1]);             \
        iter_count++;                                                   \
        break;                                                          \
        case POPRAM:                                                    \
        TranslatePopRAM(dst_buffer, (u_int64_t)code[iter_count + 1]);   \
        iter_count++;                                                   \
        break


#define CALL_JUMP_HANDLER                                               \
        CALL:                                                           \
        case JB:                                                        \
        case JE:                                                        \
        case JA:                                                        \
        case JMP:                                                       \
        JmpCallHandler (dst_buffer, &table, code[iter_count + 1],       \
                        iter_count, code[iter_count]);                  \
        iter_count++;                                                   \
        break

#define ARITHMETIC_OPERATIONS                                           \
        ADD:                                                            \
        case DIV:                                                       \
        case MUL:                                                       \
        case SUB:                                                       \
        TranslateArithmeticOp (dst_buffer, code[iter_count]);           \
        break;                                                          \
        case SQRT:                                                      \
        TranslateSqrt (dst_buffer);                                     \
        break



#define STDIO_HANDLER                                                   \
        IN:                                                             \
        TranslateIn (dst_buffer);                                       \
        break;                                                          \
        case OUT:                                                       \
        TranslateOut (dst_buffer);                                      \
        break

#define RET_HLT_HANLDER                                                 \
        RET:                                                            \
        TranslateRet (dst_buffer);                                      \
        break;                                                          \
        case HLT:                                                       \
        TranslateHlt (dst_buffer);                                      \
        break

#define LABEL_SET                                                       \
        search_res = LabelTableSearchByLabel (&table, iter_count);      \
                                                                        \
        if (search_res != NOT_FOUND)                                    \
                LinkLabel (dst_buffer,                                  \
                           &table,                                      \
                           search_res,                                  \
                           iter_count)

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

void TranslationStart (const char* const src_file_name, assembly_code* const dst_buffer, const int time_flag)
{
        auto start = std::chrono::high_resolution_clock::now(); // засекаем время

        assembly_code src_code = {};
        LoadBinaryCode (src_file_name, &src_code);
        int* code = (int*)src_code.code;
        AssemblyCodeAlignedInit (dst_buffer, PAGESIZE, MIN_DST_CODE_SIZE);

        size_t op_count = src_code.size / sizeof(int);
        dst_buffer->code += 2 * PAGESIZE;
        SetDataSegment   (dst_buffer);
        TranslateSaveRsp (dst_buffer);

        label_table table = {};
        LabelTableInit (&table);
        MakeLabelTable (&src_code, &table);

        int search_res = 0;

        for (size_t iter_count = 0; iter_count < op_count; iter_count++)
        {
                LABEL_SET;
                switch (code[iter_count])
                {
                case RET_HLT_HANLDER;
                case CALL_JUMP_HANDLER;

                case POP_HANDLER; // поменять весь pop
                case PUSH_HANDLER;

                case ARITHMETIC_OPERATIONS;

                case STDIO_HANDLER;
                }
        }

        TranslateLoadRsp (dst_buffer);
        *dst_buffer->code = (char)RET_OP_CODE;
        dst_buffer->code -= dst_buffer->position;
        // обнуляем позицию на первую команду

        if (time_flag)
        {
                auto stop     = std::chrono::high_resolution_clock::now();
                auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

                printf("Translation time: %ld ms\n", duration.count());
        }
}

void MakeLabelTable (assembly_code* const src_code, label_table* const table)
{                       // тоже все перепроверить
        size_t op_count = src_code->size / sizeof(int);
        int*       code = (int*)src_code->code;

        for (size_t iter_count = 0; iter_count < op_count; iter_count++)
        {
                switch (code[iter_count])
                {
                case PUSH:
                        iter_count += 2;
                        break;
                case CALL:
                case JB:
                case JE:
                case JA:
                case JMP:
                        LabelTableAdd (table, code[iter_count + 1], iter_count);
                        iter_count++;
                        break;
                case PUSHREG:
                case POPREG:
                case PUSHRAM:
                case POPRAM:
                        iter_count += 2;                                                // проверить
                        break;
                case RET:
                case OUT:
                case POP:
                case ADD:
                case DIV:
                case MUL:
                case SUB:
                        iter_count++;
                        break;
                }
        }
}
