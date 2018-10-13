#include <assert.h>
#include "FreeBlockRecord.h"
#include "FreeBlockLList.h"
#include "util.h"

void Init_FBR(struct FreeBlockRecord *record, struct LListRecord *llist, struct FreeBlockRecord *prev, struct FreeBlockRecord *next, size_t size_of_chunk)
{
    die_if_false(size_of_chunk >= MIN_BLOCK_SIZE, "Cannot init FBR with size that small\n");
    record->size = size_of_chunk - sizeof(size_t);
    Splice_Between(record, llist, prev, next);
}

//returns true if a split was actually performed
//wanted_size is the minimum value of size this FBR will have afterwards
//size may be slightly larger than requested
bool Split_Record(struct FreeBlockRecord *record, struct LListRecord *llist, size_t wanted_size)
{
    if(wanted_size <= record->size && wanted_size < MIN_BLOCK_SIZE) return false;
    if(wanted_size < MIN_BLOCK_SIZE) wanted_size = MIN_BLOCK_SIZE;
    die_if_false(record, "Split_Record: FBR is NULL\n");
    die_if_false(record->size >= wanted_size, "Split_Record: Cannot grow FBR by splitting it in two\n");

    if(record->size == wanted_size) return false; //Nothing to do
    if(record->size <= MIN_BLOCK_SIZE * 2) return false; //This FBR is too small to split, must be used as is

    //Actually split the record
    Init_FBR(((void*) record) + wanted_size + sizeof(size_t), llist, record, record->next, record->size - wanted_size);
    record->size = wanted_size;
    return true;
}

//Assimilate immediate neighbors
//returns the resulting record
struct FreeBlockRecord *Coalesce_If_Possible(struct FreeBlockRecord *record, struct LListRecord *llist)
{
    struct FreeBlockRecord *result = record;
    if(record->next)
    {
        die_if_false(record->next->prev == record, "Link Error\n");
        die_if_false(record < record->next, "Ordering error, next is behind this record\n");

        if((void*) record->next - record->size - sizeof(size_t) == record)
        {
            write_string(STDERR_FILENO, "coalase right\n", 50);
            record->size += record->next->size + sizeof(size_t);
            Unlink_From_LList(record->next, llist);
        }
    }
    if(record->prev)
    {
        die_if_false(record->prev->next == record, "Link Error\n");
        die_if_false(record > record->prev, "Ordering error, prev is ahead of this record\n");

        if((void*) record->prev + record->prev->size + sizeof(size_t) == record)
        {
            write_string(STDERR_FILENO, "coalase left\n", 50);
            result = record->prev;
            record->prev->size += record->size + sizeof(size_t);
            Unlink_From_LList(record, llist);
        }
    }
    die_if_false(!(llist->size>1) || result->prev || result->next, "link error\n");
    return result;
}

void Splice_Between(struct FreeBlockRecord *record, struct LListRecord *llist, struct FreeBlockRecord *left, struct FreeBlockRecord *right)
{
    record->prev = left;
    record->next = right;
    if(left && right) die_if_false(left->next == right && right->prev == left, "Splice_Between: Link error\n");
    if(!left) die_if_false(right == llist->head, "Splice_Between: if left is NULL, then record is the new head of the list. But the current head must equal right, or the splice is invalid.\n");
    if(!right) die_if_false(left == llist->tail, "Splice_Between: if right is NULL, then record is the new tail of the list. But the current tail must equal left, or the splice is invalid.\n");

    if(left) left->next = record;
    else llist->head = record;

    if(right) right->prev = record;
    else llist->tail = record;

    llist->size++;
    die_if_false(!(llist->size>1) || (record->prev || record->next), "Splice_between: link error\n");
}

void Unlink_From_LList(struct FreeBlockRecord *record, struct LListRecord *llist)
{
    die_if_false(record, "Unlink_From_LList: record is NULL\n");
    die_if_false(llist, "Unlink_From_LList: llist is NULL\n");

    struct FreeBlockRecord *prev = record->prev;
    struct FreeBlockRecord *next = record->next;

    if(llist->head == record) llist->head = next;
    if(llist->tail == record) llist->tail = prev;

    if(next)
    {
        die_if_false(next->prev == record, "Unlink_From_LList: Link error\n");
        next->prev = prev;
        record->next = NULL;
    }

    if(prev)
    {
        die_if_false(prev->next == record, "Unlink_From_LList: Link error\n");
        prev->next = next;
        record->prev = NULL;
    }
    llist->size--;
    if(prev) die_if_false(!(llist->size>1) || (prev->prev || prev->next), "Unlink_From_LList: link error\n");
    if(next) die_if_false(!(llist->size>1) || (next->prev || next->next), "Unlink_From_LList: link error\n");
}
