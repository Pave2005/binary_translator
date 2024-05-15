#ifndef TABLE
#define TABLE

#include <stdlib.h>
#include <stdio.h>
#include <nmmintrin.h>

#define MIN_LABEL_TABLE_SIZE 128
#define MIN_STACK_SIZE       16
#define NOT_FOUND            -1

struct stack_node
{
        int    label;
        int    jmp;
        size_t code_pos;
};

struct stack
{
        int         size;
        int         capacity;
        stack_node* data;
};

struct label_table
{
        int    size;
        stack* elems;
};

void        StackInit               (stack* const stack, const size_t size);
int         StackPush               (stack* const stack, const int label_pos, const int jmp_pos);
int         StackDestr              (stack* const stack);
int         LabelTableInit          (label_table* const self);
inline void LabelTableAdd           (label_table* const self, const int label, const int jmp);
inline int  LabelTableSearchByLabel (label_table* const self, const int label_pos);
size_t      GetCodePosByJmp         (label_table* const self, const int label_pos, const int jmp_pos);
size_t*     GetCodePosPtrByJmp      (label_table* const self, const int label_pos, const int jmp_pos);

#endif
