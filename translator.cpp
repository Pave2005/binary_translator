#include <stdio.h>
#include <stdlib.h>

#include "translator.h"

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

void WriteCommand (assembly_code* const dst_code, opcode operation_code)
{
        *(u_int64_t*)dst_code->code = operation_code.code;              // записываем в ячейку код команды
        dst_code->position         += operation_code.size;              // сдвигаем позицию в коде
        dst_code->code             += operation_code.size;              // сдвигаем адрес на следующий элемент
}

void TranslatePush (assembly_code* const dst_code, const u_int64_t data)
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

void TranslatePushRAM (assembly_code* const dst_code, const u_int64_t memory_indx) // проверить корректность
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
{                                                                               // не дописана

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

void TranslatePushREG (assembly_code* const dst_code, const u_int64_t reg_id)   // не дописана
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

void TranslatePopRAM (assembly_code* const dst_code, const u_int64_t memory_indx)
{
        constexpr opcode add_rsp =
        {
                .code = ADD_RSP_IMM | XMMWORD << BYTE(3),
                .size = OPSIZE(ADD_RSP_IMM)
        };

        constexpr opcode get_top =
        {
                .code = VMOVQ_XMM_RSP_IMM | XMM5_EXTEND << BYTE(3)
                        | XMMWORD << BYTE(5),
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

void TranslatePopPEG (assembly_code* const dst_code, const int reg_id)  // проверить все и протестировать
{
        constexpr u_int64_t suffix = 0x44; // 0x44 = 01000100b watch explanation of opcodes higher
        constexpr u_int64_t offset = 3;

        // Convetation to native register op code, using mask.
        u_int64_t reg_op_code = cvt_host_reg_id_to_native(reg_id, suffix, offset);

        opcode movq_xmm_rsp =
        {
                .code = VMOVQ_XMM_RSP_IMM | (reg_op_code << BYTE(3)) |
                        (XMMWORD << BYTE(5)),
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



void MakeLabelTable (assembly_code* const src_code, label_table*  const table) // не доделано
{
        size_t op_count = src_code->size / sizeof(int);
        int* code = (int*)src_code->code;
        for (size_t iter_count = 0; iter_count < op_count; ++iter_count)
        {
            switch (code[iter_count])
            {
                case PUSH:
                case PUSHREG:
                case PUSHRAM:
                case PUSHRAMREG:
                case POPRAM:
                case POPRAMREG:
                        iter_count += 2;
                        break;
                case CALL:
                case JB:
                case JE:
                case JA:
                case JMP:
                        label_table_add(table, code[iter_count + 1], iter_count);
                        iter_count++;
                        // Thinking...
                        break;
                case PUSHR:
                case POPR:
                case PUSHM:
                case POPM:
                        iter_count++;
                        break;

                case RET:
                case OUT:
                case POP:
                case ADD:
                case DIV:
                case MUL:
                case SUB:
                        break; // Nothing to do (will be optimized out by compiler)
            }
        }

}
