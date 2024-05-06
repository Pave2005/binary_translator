#include <stdint.h>

#define BYTE(val) val * 8
#define OPSIZE(op_code_name) SIZEOF_##op_code_name

const max_ir_node_size = 200;
const max_ir_bin_trans_size = 100;

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
    LB = 15,
    JB = 16,
    PUSHRAM = 17,       // положить в память
    POPRAM = 18,        // забрать из памяти
    CALL = 19,
    RET = 20,
    PUSHREG = 21,
    PUSHRAMREG = 22,    // положить в память по индексу из регистра
    POPRAMREG = 23,
    JA = 24,
    JAE = 25,
    JBE = 26,
    JE = 27,
    JNE = 28,
};

enum HOST_ASSEMBLY_REG_ID : u_int64_t
{
  AX = 1,
  BX,
  CX,
  DX
};

enum REGISTER_CODES
{
    RAX = 10,
    RBX = 11,
    RCX = 12,
    RDX = 13,
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

    MOV_R14              = 0xBE49,
    MOV_TO_STACK_R14     = 0x2434894C,
    SUB_RSP_IMM          = 0x00EC8348,
    VMOVQ_XMM5_R13_B_IMM = 0x006D7E7AC1C4,
    VMOVQ_RSP_XMM        = 0x2400D6F9C5,
    VMOVQ_XMM5_R13_D_IMM = 0xAD7E7AC1C4,
    ADD_RSP_IMM          = 0x00C48348,
    VMOVQ_XMM_RSP_IMM    = 0x0024007EFAC5,
    VMOVQ_R13_D_IMM_XMM5 = 0xADD679C1C4,
}

struct labels
{
    unsigned long long hash;
    unsigned int       adress;
};

struct assembly_code
{
    float* code;
    int    position;
    size_t size;
};

struct opcode
{
    u_int64_t code;
    int       size;
};

int AsseblyCodeInit (assembly_code* const self, const size_t bytes);
int LoadBinaryCode (const char* const src_file_name, assembly_code* const src_code_save);
void TranslatePush (assembly_code* const dst_code, const u_int64_t data);
