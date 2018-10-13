#ifndef FREEBLOCKLLIST_H
#define FREEBLOCKLLIST_H

#include <stddef.h>

struct FreeBlockRecord;

struct LListRecord
{
    struct FreeBlockRecord *head;
    struct FreeBlockRecord *tail;
    size_t size;    //num records
    size_t size_of_mmap_chunk;
};

#define MIN_LLIST_SPACE sizeof(struct LListRecord)

void Init_LList(struct LListRecord *record, size_t size_of_entire_mmap_chunk);
void *Alloc_Mem_Chunk_Of_Size(struct LListRecord *record, size_t size);
void Free_Mem_Chunk(struct LListRecord *record, void *mem_addr);

struct FreeBlockRecord *Find_Block_With_Enough_Space(struct LListRecord *record, size_t size);
void Return_Block_To_List(struct LListRecord *llist, struct FreeBlockRecord *record);

#endif