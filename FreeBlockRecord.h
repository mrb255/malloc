#ifndef FREEBLOCKRECORD_H
#define FREEBLOCKRECORD_H

#include <stddef.h>
#include <stdbool.h>

struct LListRecord;

struct FreeBlockRecord
{
    size_t data_size;    //size of the memory that may be stored in this block. When this block is allocated out, prev and next get overritten, but size does not
    struct FreeBlockRecord *prev;
    struct FreeBlockRecord *next;
};

#define MIN_BLOCK_SIZE (2*sizeof(void*))

void Init_FBR(struct FreeBlockRecord *record, struct LListRecord *llist, struct FreeBlockRecord *prev, struct FreeBlockRecord *next, size_t size_of_entire_block);
bool Split_Record(struct FreeBlockRecord *record, struct LListRecord *llist, size_t wanted_size);
struct FreeBlockRecord *Coalesce_If_Possible(struct FreeBlockRecord *record, struct LListRecord *llist);
void Splice_Between(struct FreeBlockRecord *record, struct LListRecord *llist, struct FreeBlockRecord *left, struct FreeBlockRecord *right);
void Unlink_From_LList(struct FreeBlockRecord *record, struct LListRecord *llist);

#endif