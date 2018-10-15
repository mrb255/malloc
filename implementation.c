/*  

    Copyright 2018 by

    University of Alaska Anchorage, College of Engineering.

    All rights reserved.

    Contributors:  ...
		   ...
		   ...                 and
		   Christoph Lauter

    See file memory.c on how to compile this code.

    Implement the functions __malloc_impl, __calloc_impl,
    __realloc_impl and __free_impl below. The functions must behave
    like malloc, calloc, realloc and free but your implementation must
    of course not be based on malloc, calloc, realloc and free.

    Use the mmap and munmap system calls to create private anonymous
    memory mappings and hence to get basic access to memory, as the
    kernel provides it. Implement your memory management functions
    based on that "raw" access to user space memory.

    As the mmap and munmap system calls are slow, you have to find a
    way to reduce the number of system calls, by "grouping" them into
    larger blocks of memory accesses. As a matter of course, this needs
    to be done sensibly, i.e. without spoiling too much memory.

    You must not use any functions provided by the system besides mmap
    and munmap. If you need memset and memcpy, use the naive
    implementations below. If they are too slow for your purpose,
    rewrite them in order to improve them!

    Catch all errors that may occur for mmap and munmap. In these cases
    make malloc/calloc/realloc/free just fail. Do not print out any 
    debug messages as this might get you into an infinite recursion!

    Your __calloc_impl will probably just call your __malloc_impl, check
    if that allocation worked and then set the fresh allocated memory
    to all zeros. Be aware that calloc comes with two size_t arguments
    and that malloc has only one. The classical multiplication of the two
    size_t arguments of calloc is wrong! Read this to convince yourself:

    https://cert.uni-stuttgart.de/ticker/advisories/calloc.en.html

    In order to allow you to properly refuse to perform the calloc instead
    of allocating too little memory, the __try_size_t_multiply function is
    provided below for your convenience.
    
*/

#define _GNU_SOURCE

#include <stddef.h>
#include <errno.h>
#include <sys/mman.h>
#include "FreeBlockLList.h"
#include "FreeBlockRecord.h"
#include "util.h"

/* Predefined helper functions */

static void *__memset(void *s, int c, size_t n) {
  unsigned char *p;
  size_t i;

  if (n == ((size_t) 0)) return s;
  for (i=(size_t) 0,p=(unsigned char *)s; i<=(n-((size_t) 1)); i++,p++)
  {
    *p = (unsigned char) c;
  }
  return s;
}

static void *__memcpy(void *dest, const void *src, size_t n) {
  unsigned char *pd;
  const unsigned char *ps;
  size_t i;

  if (n == ((size_t) 0)) return dest;
  for (i=(size_t) 0,pd=(unsigned char *)dest,ps=(const unsigned char *)src;
       i<=(n-((size_t) 1));
       i++,pd++,ps++) {
    *pd = *ps;
  }
  return dest;
}

/* Tries to multiply the two size_t arguments a and b.

   If the product holds on a size_t variable, sets the 
   variable pointed to by c to that product and returns a 
   non-zero value.
   
   Otherwise, does not touch the variable pointed to by c and 
   returns zero.

   This implementation is kind of naive as it uses a division.
   If performance is an issue, try to speed it up by avoiding 
   the division while making sure that it still does the right 
   thing (which is hard to prove).

*/
static int __try_size_t_multiply(size_t *c, size_t a, size_t b) {
  size_t t, r, q;

  /* If any of the arguments a and b is zero, everthing works just fine. */
  if ((a == ((size_t) 0)) ||
      (a == ((size_t) 0))) {
    *c = a * b;
    return 1;
  }

  /* Here, neither a nor b is zero. 

     We perform the multiplication, which may overflow, i.e. present
     some modulo-behavior.

  */
  t = a * b;

  /* Perform Euclidian division on t by a:

     t = a * q + r

     As we are sure that a is non-zero, we are sure
     that we will not divide by zero.

  */
  q = t / a;
  r = t % a;

  /* If the rest r is non-zero, the multiplication overflowed. */
  if (r != ((size_t) 0)) return 0;

  /* Here the rest r is zero, so we are sure that t = a * q.

     If q is different from b, the multiplication overflowed.
     Otherwise we are sure that t = a * b.

  */
  if (q != b) return 0;
  *c = t;
  return 1;
}

/* End of predefined helper functions */

/* Your helper functions 

   You may also put some struct definitions, typedefs and global
   variables here. Typically, the instructor's solution starts with
   defining a certain struct, a typedef and a global variable holding
   the start of a linked list of currently free memory blocks. That 
   list probably needs to be kept ordered by ascending addresses.

*/

#define MAX_LLISTS 500000
struct LListRecord *llists[MAX_LLISTS] = {0};

size_t Get_Empty_Index()
{
  size_t n = 0;
  for(n = 0; n < MAX_LLISTS; n++)
    if(llists[n] == NULL)
      return n;
  die_if_false(0==1, "Out of space for llists\n");
  return -1;
}

//Try to alloc using existing llists
void *Try_Alloc(size_t size)
{
  void *mem;
  for(size_t n = 0; n < MAX_LLISTS; n++)
  {
    if(!llists[n]) continue;
    mem = Alloc_Mem_Chunk_Of_Size(llists[n], size);
    if(mem) return mem;
  }
  return NULL;
}

#define SIZE_OF_BOOKEEPING (sizeof(struct LListRecord) + sizeof(struct FreeBlockRecord))

size_t Calculate_MMap_Size(size_t requested_size)
{
  requested_size += SIZE_OF_BOOKEEPING;
  if(requested_size < SIZE_OF_BOOKEEPING * 50)
    requested_size = SIZE_OF_BOOKEEPING * 50;
  //TODO: make more efficent
  return requested_size;
}

bool Find_Index_Of_LList_Containing_FBR(void *fbr, size_t *index)
{
  void *block_start;
  void *block_end;
  for(size_t n = 0; n < MAX_LLISTS; n++)
  {
    if(!llists[n]) continue;
    block_start = (void*) llists[n] + sizeof(struct LListRecord);
    block_end = (void*) llists[n] + llists[n]->size_of_mmap_chunk - sizeof(struct FreeBlockRecord);
    if(block_start <= fbr && fbr <= block_end)
    {
      *index = n;
      return true;
      //return llists[n];
    }
  }

  /*for(size_t n = 0; n < MAX_LLISTS; n++)
  {
    if(!llists[n]) continue;
    block_start = (void*) llists[n] + sizeof(struct LListRecord);
    block_end = (void*) llists[n] + llists[n]->size_of_mmap_chunk - sizeof(struct FreeBlockRecord);
    char buffer[50], buffer2[50], buffer3[50], buffer4[50];
    convert_integer(buffer, (long) fbr, 16, 0);
    convert_integer(buffer2, (long) block_start, 16, 0);
    convert_integer(buffer3, (long) block_end, 16, 0);
    convert_integer(buffer4, n, 10, 1);
    write_strings(STDERR_FILENO, 100, 9, "N: ", buffer4, " Pointers: ", buffer, " ", buffer2, " ", buffer3, "\n");
  }*/

  return false;
}

#define MAX(X, Y) (((X) < (Y)) ? (Y) : (X))
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

//#define MIN_MMAP_SPACE_FOR_LLIST (sizeof(struct LListRecord) + sizeof(struct FreeBlockRecord))

/* End of your helper functions */

/* Start of the actual malloc/calloc/realloc/free functions */

void __free_impl(void *);

void *__malloc_impl(size_t size) {
  if(size == 0) return NULL;
  char buffer3[50] = {0};
  if(size % 8 != 0)
    size = size + 8 - (size % 8);
  convert_integer(buffer3, size, 10, 0);
  write_strings(STDERR_FILENO, 100, 3, "Alloc of size: ", buffer3, "\n");

  void *retvalue;
  retvalue = Try_Alloc(size);
  if(retvalue) return retvalue;
        write_string(STDERR_FILENO, "test\n", 10);

  size_t index = Get_Empty_Index();
  size_t calculated_size = Calculate_MMap_Size(size); //MAX(size + , MIN_MMAP_SPACE_FOR_LLIST);

  char buffer[50] = {0};
  char buffer2[50] = {0};
  convert_integer(buffer, calculated_size, 10, 0);
  convert_integer(buffer2, size, 10, 0);
  write_strings(STDERR_FILENO, 100, 5, "Calculated size: ", buffer, " size: ", buffer2, "\n");

  llists[index] = mmap(NULL, calculated_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  die_if_false(llists[index], "mmap return NULL\n");
  Init_LList(llists[index], calculated_size);
  die_if_false(llists[index]->length == 1, "bad init\n");
  retvalue = Alloc_Mem_Chunk_Of_Size(llists[index], size);
  die_if_false(retvalue, "retvalue is NULL\n");
  return retvalue;
}

void *__calloc_impl(size_t nmemb, size_t size) {
  void *mem = __malloc_impl(nmemb * size);
  __memset(mem, 0, nmemb * size);
  return mem;  
}

void *__realloc_impl(void *ptr, size_t size) {
  void *mem;

  if(ptr == NULL)
  {
    if(size != 0) return __malloc_impl(size);
  }
  if(size == 0)
  {
    __free_impl(ptr);
    return NULL;
  }
  
  size_t *old_size = ptr - sizeof(size_t);

  char buffer[50] = {0};
  char buffer2[50] = {0};
  convert_integer(buffer, size, 10, 0);
  convert_integer(buffer2, *old_size, 10, 0);
  write_strings(STDERR_FILENO, 100, 5, "Sizes: ", buffer, " ", buffer2, "\n");
  mem = __malloc_impl(size);
  __memcpy(mem, ptr, MIN(*old_size, size)); //TODO: what if size is smaller?
  __free_impl(ptr);
  return mem;
}

void __free_impl(void *ptr) {
  if(!ptr) return;

  struct FreeBlockRecord *fbr = ptr - sizeof(size_t);
  size_t llist_index;
  
  //die_if_false(Find_Index_Of_LList_Containing_FBR(fbr, &llist_index), "__free_impl: Cannot find containing llist\n");
  if(!Find_Index_Of_LList_Containing_FBR(fbr, &llist_index)) return;

  Free_Mem_Chunk(llists[llist_index], ptr);
  if(llists[llist_index]->length == 1)
  {
    if(llists[llist_index]->size_of_mmap_chunk - llists[llist_index]->head->data_size - sizeof(size_t) == sizeof(struct LListRecord))
    {
      write_string(STDERR_FILENO, "Unmapping empty llist\n", 50);
      //hexDump(llists[llist_index], llists[llist_index]->size_of_mmap_chunk);
      if(munmap(llists[llist_index], llists[llist_index]->size_of_mmap_chunk) == -1)
      {
        write_strings(STDERR_FILENO, 100, 3, "munmap failed: ", strerror(errno), "\n");
        _exit(1);
      }
      llists[llist_index] = NULL;
    }
  }
}

/* End of the actual malloc/calloc/realloc/free functions */

