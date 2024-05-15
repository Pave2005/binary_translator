#include "label_table.h"

#define STACK self->elems

#define CODE_POS(count)  STACK[idx].data[count].code_pos
#define JMP_POS(count)   STACK[idx].data[count].jmp

void StackInit (stack* const stack, const size_t size)
{
        stack->data     = (stack_node*)calloc (size, sizeof (stack_node));
        stack->size     = 0;
        stack->capacity = size;
}

int StackPush (stack* const stack, const int label_pos, const int jmp_pos)
{
        if (stack->size >= stack->capacity)
        {
                stack->data = (stack_node*)realloc (stack->data, stack->capacity * 2 * sizeof (stack_node));
                if (stack->data == nullptr)
                        return 1;
                stack->capacity *= 2;
        }

        stack->data[stack->size].label = label_pos;
        stack->data[stack->size++].jmp = jmp_pos;

        return 0;
}

int StackDestr (stack* const stack)
{
        free(stack->data);
        stack->data = nullptr;
        stack->size = stack->capacity = 0;
        return 0;
}


int LabelTableInit (label_table* const self)
{
        self->elems = (stack*)calloc (MIN_LABEL_TABLE_SIZE, sizeof (stack));      // выделили множество таблицы

        for (int iter_count = 0; iter_count < MIN_LABEL_TABLE_SIZE; iter_count++)
                StackInit(&self->elems[iter_count], MIN_STACK_SIZE);              // выделили подмножества таблицы

        return 0;
}

inline void LabelTableAdd (label_table* const self, const int label, const int jmp)
{
        int idx = _mm_crc32_u32 (label, 0xDED) % MIN_LABEL_TABLE_SIZE;
        StackPush (&self->elems[idx], label, jmp);
}

inline int LabelTableSearchByLabel (label_table* const self, const int label_pos)
{
        int idx = _mm_crc32_u32 (label_pos, 0xDED) % MIN_LABEL_TABLE_SIZE;
        if (STACK[idx].size == 0)
                return NOT_FOUND;
        return idx;  // возвращаем индекс подмножества
};

size_t GetCodePosByJmp (label_table* const self, const int label_pos, const int jmp_pos)
{
        int idx = _mm_crc32_u32 (label_pos, 0xDED) % MIN_LABEL_TABLE_SIZE;
        for (int iter_count = 0; iter_count < STACK[idx].size; iter_count++)
                if (jmp_pos == JMP_POS(iter_count))
                        return CODE_POS(iter_count);
        return 1;
}

size_t* GetCodePosPtrByJmp (label_table* const self, const int label_pos, const int jmp_pos)
{
        int idx = _mm_crc32_u32 (label_pos, 0xDED) % MIN_LABEL_TABLE_SIZE;
        for (int iter_count = 0; iter_count < STACK[idx].size; iter_count++)
                if (jmp_pos == JMP_POS(iter_count))
                        return &CODE_POS(iter_count);
        return nullptr;
}
