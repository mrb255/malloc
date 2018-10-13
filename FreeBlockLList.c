#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "FreeBlockLList.h"
#include "FreeBlockRecord.h"
#include "util.h"
#include <stdio.h>

void hexDump (void *addr, int len) {
    int i;
    unsigned char buff[17];       // stores the ASCII data
    unsigned char *pc = addr;     // cast to make the code cleaner.

    for (i = 0; i < len; i++) {
        if ((i % 16) == 0) {
            if (i != 0)
                printf ("  %s\n", buff);
            printf ("  %04x ", i);
        }

        printf (" %02x", pc[i]);

        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    printf ("  %s\n", buff);
}

#define TEST_SIZE 500
int main()
{
    char mem[TEST_SIZE] = {0};

    Init_LList((void *) mem, TEST_SIZE);
    hexDump(mem, TEST_SIZE);
    unsigned long *d = Alloc_Mem_Chunk_Of_Size((void *) mem, sizeof(unsigned long));
    die_if_false(d, "Alloc for d failed\n");
    *d = 0xDEADC0DEDEADC0DE;
    unsigned long *e = Alloc_Mem_Chunk_Of_Size((void *) mem, sizeof(unsigned long) * 10);
    die_if_false(e, "Alloc for e failed\n");
    for(int n = 0; n < 10; n++)
        e[n] = 0xBEEFFEEDBEEFFEED;
    unsigned long *f = Alloc_Mem_Chunk_Of_Size((void *) mem, sizeof(unsigned long));
    die_if_false(f, "Alloc for f failed\n");
    *f = 0xF00DBEE4F00dBEE4;
    printf("\n");
    hexDump(mem, TEST_SIZE);

    printf("e free\n");
    Free_Mem_Chunk((struct LListRecord *) mem, e);
    hexDump(mem, TEST_SIZE);

    printf("d free\n");
    Free_Mem_Chunk((struct LListRecord *) mem, d);
    hexDump(mem, TEST_SIZE);
    printf("f free\n");
    Free_Mem_Chunk((struct LListRecord *) mem, f);
    hexDump(mem, TEST_SIZE);

    return 0;
}

void Init_LList(struct LListRecord *llist, size_t size_of_entire_mmap_chunk)
{
    die_if_false(llist, NULL);
    die_if_false(size_of_entire_mmap_chunk >= sizeof(struct LListRecord) + sizeof(struct FreeBlockRecord), "Space too small for LListRecord\n");

    llist->size = 0;
    llist->head = llist->tail = NULL;
    llist->size_of_mmap_chunk = size_of_entire_mmap_chunk;

    Init_FBR((void *) llist + sizeof(struct LListRecord),
            llist,
            NULL,
            NULL,
            size_of_entire_mmap_chunk - sizeof(struct LListRecord));
}

void *Alloc_Mem_Chunk_Of_Size(struct LListRecord *record, size_t size)
{
    struct FreeBlockRecord *chunk = Find_Block_With_Enough_Space(record, size);

    if(!chunk) return NULL;
    Split_Record(chunk, record, size);              //split the block so it contains only the min space
    Unlink_From_LList(chunk, record);               //remove this free slace record from the list
    return (void*)chunk + sizeof(size_t);           //the size field in the FreeBlockRecord gets preserved, prev/next get overwritten
}                                                   //officially, the c compilere cannot change the order of vars in a struct, but may add padding

void Free_Mem_Chunk(struct LListRecord *llist, void *mem_addr)
{
    die_if_false(llist,  "Free_Mem_Chunk: llist is NULL\n");
    die_if_false(mem_addr, "Free_Mem_Chunk: cannot free null pointer\n");

    struct FreeBlockRecord *fbr = (mem_addr - sizeof(size_t));
    fbr->next = fbr->prev = NULL;
    Return_Block_To_List(llist, fbr);
}

struct FreeBlockRecord *Find_Block_With_Enough_Space(struct LListRecord *record, size_t size)
{
    die_if_false(record, "Find_Block_With_Enough_Space: record is NULL\n");

    if(!record->head) return NULL;
    struct FreeBlockRecord *fb_record = record->head;
    do
    {
        if(fb_record->size >= size)
            return fb_record;
        else
            fb_record = fb_record->next;
    } while(fb_record);

    return NULL;
}

void Return_Block_To_List(struct LListRecord *llist, struct FreeBlockRecord *record)
{
    die_if_false(llist, "Return_Block_To_List: llist is NULL\n");
    die_if_false(record, "Return_Block_To_List: record is NULL\n");

    struct FreeBlockRecord *before = NULL;
    struct FreeBlockRecord *after = NULL;

    for(struct FreeBlockRecord *current = llist->head; current; current = current->next)
    {
        if(current < record)
            before = current;
        if(current > record)    //llist is in memory order
        {
            before = current->prev;
            after = current;
            break;
        }
    }
    Splice_Between(record, llist, before, after);
    Coalesce_If_Possible(record, llist);
}