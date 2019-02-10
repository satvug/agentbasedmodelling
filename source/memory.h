#ifndef MEMORY_H

void zero_size(void *memory, u64 size)
{
    u8 *cursor = (u8 *)memory;
    while(size--)
    {
        *cursor++ = 0;
    }
}
#define zero_struct(structure) zero_size(structure, sizeof(*structure))

/*
// TODO: optimize it
void memory_copy(void *src, void *dst, size_t size_in_bytes)
{
	u8 *src_cursor = (u8 *)src;
	u8 *dst_cursor = (u8 *)dst;

	for(size_t i = 0; i < size_in_bytes; ++i)
	{
		dst_cursor[i] = src_cursor[i];
	}
}
*/

void memory_copy(void *src, void *dst, size_t size_in_bytes)
{
	__m128 *src128 = (__m128 *)src;
	__m128 *dst128 = (__m128 *)dst;
	size_t size128 = size_in_bytes & 0xfffffffffffff000;
	for(size_t i = 0; i < size128; ++i)
	{
		*dst128++ = *src128++;
	}

	u8 *src008 = (u8 *)src128;
	u8 *dst008 = (u8 *)dst128;
	size_t size008 = size_in_bytes & 0x0000000000000fff;
	for(size_t i = 0; i < size008; ++i)
	{
		*dst008++ = *src008++;
	}
}
#define array_copy(src, dst, type, count) memory_copy(src, dst, sizeof(type) * count)

bool memory_compare(void *a, void *b, size_t size)
{
	bool result = true;

	u8 *cursor_a = (u8 *)a;
	u8 *cursor_b = (u8 *)b;
	for(size_t i = 0; i < size && result; ++i)
	{
		result = cursor_a[i] == cursor_b[i];
	}

	return result;
}

struct MemoryBlock
{
    void *base;
    size_t size;
    size_t used;
};

inline size_t bytes_available(MemoryBlock *block)
{
    size_t result = 0;

    if(block->size > block->used)
    {
        result = block->size - block->used;
    }

    return result;
}

inline void *get_offset(MemoryBlock *block)
{
    void *result = (void *)((size_t)block->base + block->used);

    return result;
}

// Just to make the usage more explicit, I guess
enum MemoryFlags
{
    memory_flag_none  = 0,
    memory_flag_clear = 1 << 0,
};

void *push_size(MemoryBlock *block, size_t size,
                s32 alignment = 1, u32 flags = memory_flag_clear)
{
    void *result = 0;

	u64 block_offset = (u64)get_offset(block);
	u64 alignment_offset = block_offset % alignment;
	if(alignment_offset > 0)
	{
		alignment_offset = alignment - alignment_offset;
	}
	
    if(bytes_available(block) >= size + alignment_offset)
    {
        result = (void *)(block_offset + alignment_offset);
        if(flags & memory_flag_clear)
        {
            zero_size(result, size);
        }
        block->used += alignment_offset + size;
    }

    return result;
}

#define push_struct(block, type, ...)        (type *)push_size(block, sizeof(type), ##__VA_ARGS__)
#define push_array(block, type, count, ...)  (type *)push_size(block, (count) * sizeof(type), ##__VA_ARGS__)

void *get_remaining_memory(MemoryBlock *block, s32 alignment = 1, s32 flags = memory_flag_clear)
{
    void *result = 0;

	u64 block_offset = (u64)get_offset(block);
	u64 alignment_offset = block_offset % alignment;
	if(alignment_offset > 0)
	{
		alignment_offset = alignment - alignment_offset;
	}

	u64 size = bytes_available(block - alignment_offset);
	result = (void *)(block_offset + alignment_offset);
	if(flags & memory_flag_clear)
	{
		zero_size(result, size);
	}
	block->used += size;

    return result;
}

MemoryBlock create_subblock(MemoryBlock *block, size_t size)
{
    MemoryBlock result = {};

    result.base = push_size(block, size, 1, memory_flag_none);

    if(result.base)
    {
        result.size = size;
    }

    return result;
}

//
//
//

struct TmpMemory
{
    MemoryBlock *memblock;
    size_t used;

    TmpMemory(MemoryBlock *block)
    {
        if(block)
        {
            memblock = block;
            used = block->used;
        }
    }
    
    ~TmpMemory()
    {
        if(memblock)
        {
            memblock->used = used;
        }
    }
};

#define begin_tmp_memory__(block, counter) TmpMemory tmp ## counter(block)
#define begin_tmp_memory_(block, counter) begin_tmp_memory__(block, counter)
#define begin_tmp_memory(block) begin_tmp_memory_(block, __COUNTER__)
#define begin_tmp_memory_scope(block) { begin_tmp_memory(block)
#define end_tmp_memory_scope() }
//#define end_tmp_memory(tmp_block) tmp_block.~TmpMemory();

//
//
//

struct DynamicBlock
{    
    DynamicBlock *prev;
    DynamicBlock *next;

    // Does not include the header itself
    size_t size;
};

// Return value is a pointer to sentinel of doubly-linked
// list of free memory blocks. Initial list consists of
// one block with 'size' bytes of free memory
DynamicBlock *dynamic_memory_init(MemoryBlock *block, size_t size)
{
    size_t total_size = size + 2 * sizeof(DynamicBlock);

    DynamicBlock *result = (DynamicBlock *)push_size(block, total_size);

    if(result)
    {
        DynamicBlock *dynamic_block = result + 1;

        dynamic_block->size = size;
        dynamic_block->prev = dynamic_block->next = result;
        result->prev = result->next = dynamic_block;
    }

    return result;
}

DynamicBlock *dynamic_memory_init(MemoryBlock *block)
{
    DynamicBlock *result = 0;
    size_t size = bytes_available(block);

    if(size > 2 * sizeof(DynamicBlock))
    {
        result = dynamic_memory_init(block, size - 2 * sizeof(DynamicBlock));
    }

    return result;
}

void *alloc_size(DynamicBlock *sentinel, size_t size)
{
    void *result = 0;
    
    size_t total_size = sizeof(DynamicBlock) + size;

    for(DynamicBlock *block = sentinel->next;
                      block != sentinel && !result;
                      block = block->next)
    {
        if(block->size == size)
        {
            result = block + 1;

            block->prev->next = block->next;
            block->next->prev = block->prev;
            block->prev = block->next = sentinel;
        }
        else if(block->size > total_size)
        {
            result = block + 1;
            size_t end = (size_t)result + size;

            DynamicBlock *new_block = (DynamicBlock *)end;
            new_block->size = block->size - total_size;
            new_block->prev = block->prev;
            new_block->next = block->next;
            new_block->prev->next = new_block->next->prev = new_block;

            block->size = size;
            block->prev = block->next = sentinel;            
        }
    }

    return result;
}

#define alloc_struct(block, type)  (type *)alloc_size(block, sizeof(type))
#define alloc_array(block, type, count)   (type *)alloc_size(block, (count) * sizeof(type))

void memory_free(void *memory)
{
    size_t base = (size_t)memory - sizeof(DynamicBlock);

    DynamicBlock *block = (DynamicBlock *)base;
    DynamicBlock *sentinel = block->next;

    if(block->prev == sentinel)
    {
        DynamicBlock  *prev, *next = sentinel->next;

        while(next != sentinel && next <= block)
        {
            next = next->next;
        }

        prev = next->prev;

        block->prev = prev;
        block->next = next;

        size_t prev_end = (size_t)prev + sizeof(DynamicBlock) + prev->size;
        size_t cur_start = (size_t)block;
        size_t cur_end = (size_t)memory + block->size;
        size_t next_start = (size_t)next;

        if(prev != sentinel && prev_end == cur_start)
        {
            prev->size += block->size + sizeof(DynamicBlock);
        }
        else
        {
            next->prev = prev->next = block;
        }

        if(cur_end == next_start)
        {
            block = next;

            block->next->prev = block->prev;
            block->prev->next = block->next;

            block->prev->size += block->size + sizeof(DynamicBlock);
        }
    }
    else
    {
        // Attempt of freeing the memory that has not been allocated
        // or something weird is happening
        // or someone broke the header
    }
}

#define MEMORY_H
#endif
