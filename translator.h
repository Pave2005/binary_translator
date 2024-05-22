#ifndef TRANS
#define TRANS

#include <stdint.h>
#include <cstdint>
#include <sys/types.h>
#include <sys/mman.h>

#include "label_table.h"

typedef uint64_t u_int64_t;

#define PAGESIZE            4096
#define TRANSLATE_PUSH_SIZE 23
#define MIN_DST_CODE_SIZE   2 << 14

#define BYTE(val) val * 8
#define OPSIZE(op_code_name) SIZEOF_##op_code_name
#define SPACE(num)                                  \
        asmcode sep =                               \
        {                                           \
                .code = (char*)calloc (num, 1),     \
                .size = num                         \
        };                                          \
        for (int i = 0; i < num; i++)               \
                memcpy (sep.code + i, "\n", 1);     \
        WriteAsmCommand (dst_code, sep)

#define INIT_ASM_ELEM(name)                                 \
        asmcode name =                                      \
        {                                                   \
                .code = (char*)calloc (500, sizeof (char)), \
                .size = 0                                   \
        };
// нужно очищать фрагмент памяти
const int max_ir_node_size = 200;
const int max_ir_bin_trans_size = 100;

enum WORD_SIZE : u_int64_t
{
    DWORD   = 4,
    QWORD   = 8,
    XMMWORD = 16,
};

enum REG_MASK : u_int64_t
{
    XMM0_EXTEND = 0x44,
    XMM5_BASE   = 0x2C,
    XMM5_EXTEND = 0x6C
};

enum ISA
{
    HLT = -1,
    PUSH = 1,
    DIV = 2,
    SUB = 3,
    OUT = 4,
    ADD = 5,
    MUL = 6,
    SQRT = 7,
    IN = 8,
    POP = 9,
    JMP = 14,
    JB = 16,
    PUSHRAM = 17,
    POPRAM = 18,
    CALL = 19,
    RET = 20,
    PUSHREG = 21,
    PUSHRAMREG = 22,
    POPRAMREG = 23,
    JA = 24,
    JAE = 25,
    JBE = 26,
    JE = 27,
    JNE = 28,
};

enum REGISTER_CODES
{
    AX = 10,
    BX = 11,
    CX = 12,
    DX = 13,
};

enum VCMPPD_COMPARISONS_CODE : u_int64_t
{
    EQUAL = 0,
    LESS = 17,
    GREATER = 30,
};

enum X86_ASSEMBLY_OPCODES : u_int64_t
{
    RAX = 0,
    RBX = 3,
    RCX = 1,
    RDX = 2,

    XMM0 = RAX,
    XMM1 = RCX,
    XMM2 = RDX,
    XMM3 = RBX,
    XMM4,

    MOV_R14                 = 0xBE49,
    RET_OP_CODE             = 0xC3,
    CALL_OP_CODE            = 0x00000000E8,
    MOV_TO_STACK_R14        = 0x2434894C,
    SUB_RSP_IMM             = 0x00EC8348,
    VMOVQ_XMM5_R13_B_IMM    = 0x006D7E7AC1C4,
    VMOVQ_RSP_XMM           = 0x2400D6F9C5,
    VMOVQ_XMM5_R13_D_IMM    = 0xAD7E7AC1C4,
    ADD_RSP_IMM             = 0x00C48348,
    VMOVQ_XMM_RSP_IMM       = 0x0024007EFAC5,
    VMOVQ_R13_D_IMM_XMM5    = 0xADD679C1C4,
    VMOVQ_R13_B_IMM_XMM5    = 0x006DD679C1C4,
    VMOVQ_RSP_IMM_XMM       = 0x002400D6F9C5,
    ADDSD_XMM_RSP_IMM       = 0x002400580FF2,
    SUBSD_XMM_RSP_IMM       = 0x0024005C0FF2,
    MULSD_XMM_RSP_IMM       = 0x002400590FF2,
    DIVSD_XMM_RSP_IMM       = 0x0024005E0FF2,
    VSQRTPD_XMM0_XMM0       = 0xC051F9C5,
    MOV_RSP_R15             = 0xFC894C,
    MOV_RDI_RSP             = 0xE78948,
    LEA_RDI_RSP_16          = 0x10247C8D48,
    JMP_REL32               = 0x00000000E9,
    JE_REL32                = 0x00000000840F,
    VCMPPD_XMM5_XMM0_XMM5   = 0x00EDC2F9C5,
    MOVMSKPD_R14D_XMM5      = 0xF5500F4466,
    CMP_R14D_1              = 0x01FE8341,
    CMP_R14D_3              = 0x03FE8341,
    MOV_R13                 = 0xBD49,
    MOV_R15_RSP             = 0xE78949,
};

enum X86_ASSEMBLY_OPCODES_SIZE
{
    SIZEOF_VMOVQ_RSP_XMM         = 5,
    SIZEOF_VMOVQ_RSP_IMM_XMM     = 6,
    SIZEOF_SUB_RSP_IMM           = 4,
    SIZEOF_ADD_RSP_IMM           = 4,
    SIZEOF_ADDSD_XMM_RSP_IMM     = 6,
    SIZEOF_MOV_R15_RSP           = 3,
    SIZEOF_MOV_RSP_R15           = 3,
    SIZEOF_CALL_OP_CODE          = 5,
    SIZEOF_MOV_R14               = 2,
    SIZEOF_MOV_TO_STACK_R14      = 4,
    SIZEOF_VMOVQ_XMM_RSP_IMM     = 6,
    SIZEOF_VMOVQ_XMM5_R13_B_IMM  = 6,
    SIZEOF_VMOVQ_XMM5_R13_D_IMM  = 5,
    SIZEOF_MOV_R13               = 2,
    SIZEOF_JMP_REL32             = 5,
    SIZEOF_JE_REL32              = 6,
    SIZEOF_VCMPPD_XMM5_XMM0_XMM5 = 5,
    SIZEOF_MOVMSKPD_R14D_XMM5    = 5,
    SIZEOF_CMP_R14D_1            = 4,
    SIZEOF_CMP_R14D_3            = 4,
    SIZEOF_MOV_RDI_RSP           = 3,
    SIZEOF_MOV_EDI_0             = 5,
    SIZEOF_VSQRTPD_XMM0_XMM0     = 4,
    SIZEOF_LEA_RDI_RSP_16        = 5,
    SIZEOF_VMOVQ_R13_B_IMM_XMM5  = 6,
    SIZEOF_VMOVQ_R13_D_IMM_XMM5  = 5,
    SIZEOF_RET                   = 1
};

struct labels
{
    unsigned long long hash;
    unsigned int       adress;
};

struct assembly_code
{
    char*  code;
    int    position;
    size_t size;
};

struct asmcode
{
    char* code;
    int size;
};

struct opcode
{
    u_int64_t code;
    int       size;
};

union cvt_u_int64_t_int
{
    size_t       rel_addr;
    u_int64_t extended_address;
};

extern "C" int float_printf            (float* value);
extern "C" int float_scanf             (float* value);
int         AsseblyCodeInit            (assembly_code* const self, const size_t bytes);
int         LoadBinaryCode             (const char* const src_file_name, assembly_code* src_code_save);
inline void WriteCommand               (assembly_code* const dst_code, opcode operation_code);
inline void WriteAsmCommand            (assembly_code* const dst_code, asmcode assembler_code);
inline void TranslatePushToAsm         (assembly_code* const dst_code, const u_int64_t data);
inline void TranslatePush              (assembly_code* const dst_code, const u_int64_t data);
inline void TranslatePushRAMToAsm      (assembly_code* const dst_code, const u_int64_t memory_indx);
inline void TranslatePushRAM           (assembly_code* const dst_code, const u_int64_t memory_indx);
u_int64_t   cvt_host_reg_id_to_native  (const int host_reg_id, const u_int64_t suffix, const u_int64_t offset);
inline void TranslatePushREGToAsm      (assembly_code* const dst_code, const u_int64_t reg_id);
inline void TranslatePushREG           (assembly_code* const dst_code, const u_int64_t reg_id);
inline void TranslatePopRAMToAsm       (assembly_code* const dst_code, const u_int64_t memory_indx);
inline void TranslatePopRAM            (assembly_code* const dst_code, const u_int64_t memory_indx);
inline void TranslatePopREGToAsm       (assembly_code* const dst_code, const int reg_id);
inline void TranslatePopREG            (assembly_code* const dst_code, const int reg_id);
inline void TranslatePopToAsm          (assembly_code* const dst_code);
inline void TranslatePop               (assembly_code* const dst_code);
inline void TranslateArithmeticOpToAsm (assembly_code* const dst_code, const int op_id);
inline void TranslateArithmeticOp      (assembly_code* const dst_code, const int op_id);
inline void TranslateSqrtToAsm         (assembly_code* const dst_code);
inline void TranslateSqrt              (assembly_code* const dst_code);
inline void TranslateRetToAsm          (assembly_code* const dst_code);
inline void TranslateRet               (assembly_code* const dst_code);
inline void TranslateLoadRsp           (assembly_code* const dst_code);
inline void TranslateHltToAsm          (assembly_code* const dst_code);
inline void TranslateHlt               (assembly_code* const dst_code);
inline void TranslateInToAsm           (assembly_code* const dst_code);
inline void TranslateIn                (assembly_code* const dst_code);
inline void TranslateOutToAsm          (assembly_code* const dst_code);
inline void TranslateOut               (assembly_code* const dst_code);
int*        MakeLabelTableToAsm        (assembly_code* const src_code); // переделать
void        MakeLabelTable             (assembly_code* const src_code, label_table* const table);
void        CheckNSetLabel             (assembly_code* const dst_code, int* jmp_pos, const size_t iter_count);
void        SetLabel                   (assembly_code* const dst_code, const int label_pos);
void        ExecuteStart               (char* const execution_buffer);
void        OptionHandling             (int option_mask, const char* const file_name);
inline void OptionDetecting            (const char* const option, int* const option_mask);
void        Handler                    (int argc, char* argv[]);
void        TranslationStart           (const char* const src_file_name, assembly_code* const dst_buffer, assembly_code* const asm_dst_buffer, const int mode);
void        LinkLabel                  (assembly_code* const dst_code, label_table* const table, const int search_idx, const int label_pos);
inline void TranslateSaveRsp           (assembly_code* const dst_code);
inline void SetDataSegment             (assembly_code* const dst_code);
inline void TranslateCycle             (assembly_code* const dst_code, label_table* const table, const int label_pos,
                                        const int        jmp_call_pos, const int  jmp_call_code);
inline void JmpCallHandler             (assembly_code* const dst_code, label_table* const table, const int label_pos,
                                        const int        jmp_call_pos, const int  jmp_call_code);
inline void TranslateAheadJmpCall      (assembly_code* const dst_code, label_table* const table, const int label_pos,
                                        const int        jmp_call_pos, const int  jmp_call_code);
inline void TranslateTwoPopForCmp      (assembly_code* const dst_code, const int  jmp_call_code);
opcode      TranslateJmpCall           (assembly_code* const dst_code, const int  jmp_call_code);
void        TranslateRel32Label        (assembly_code* const dst_code, const size_t jmp_pos);
inline void TranslateJmpCallToAsm      (assembly_code* const dst_code, const int jmp_type, int* code);
inline void WriteHeader                (assembly_code* const dst_code);
inline void WriteEnd (assembly_code* const dst_code);

#endif
