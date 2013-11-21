/****************************************************************************
 * Copyright (C) 2013 by Matteo Franchin                                    *
 *                                                                          *
 * This file is part of Box.                                                *
 *                                                                          *
 *   Box is free software: you can redistribute it and/or modify it         *
 *   under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 3 of the License, or   *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   Box is distributed in the hope that it will be useful,                 *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Lesser General Public License for more details.                    *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with Box.  If not, see <http://www.gnu.org/licenses/>.   *
 ****************************************************************************/

#include <box/allocpool_priv.h>
#include <box/mem.h>

#include <string.h>
#include <assert.h>


/* Create a new allocation pool. */
BCAllocPool *BCAllocPool_Create(uint32_t initial_capacity)
{
  BCAllocPool *pool = Box_Mem_Alloc(sizeof(BCAllocPool));
  if (pool)
    BCAllocPool_Init(pool, initial_capacity);
  return pool;
}

/* Destroy the given allocation pool. */
void BCAllocPool_Destroy(BCAllocPool *pool)
{
  BCAllocPool_Finish(pool);
  Box_Mem_Free(pool);
}

/* Initialise an allocation pool. */
BCAllocPool *BCAllocPool_Init(BCAllocPool *pool, uint32_t initial_capacity)
{
  BCAllocSubPool *sp = & pool->sub_pool;
  void *items;

  assert(initial_capacity > 0);

  items = Box_Mem_Alloc(initial_capacity);
  if (!items)
    return NULL;

  sp->next = NULL;
  sp->items = items;
  sp->capacity = initial_capacity;
  sp->free_space = initial_capacity;
  pool->frag1_size = 0;
  pool->frag2_size = 0;
  pool->sub_pool1 = NULL;
  return pool;
}

/* Finalise an allocation pool. */
void BCAllocPool_Finish(BCAllocPool *pool)
{
  BCAllocSubPool *sp = pool->sub_pool.next;
  Box_Mem_Free(pool->sub_pool.items);
  while (sp) {
    BCAllocSubPool *next_sp = sp->next;
    Box_Mem_Free(sp->items);
    Box_Mem_Free(sp);
    sp = next_sp;
  }
}

void *BCAllocPool_Alloc(BCAllocPool *pool, uint32_t size)
{
  return BCAllocPool_Alloc_Aligned(pool, size, __alignof__(void *));
}

/* Attempt allocation using the pre-existing pools, otherwise fail and return
 * NULL.
 */
static void *My_Try_Alloc(BCAllocSubPool *sp, uint32_t size,
                          unsigned int alignment)
{
  unsigned int alignment_mask = alignment - 1;

  /* Check that alignment is a power of two. */
  assert((alignment & alignment_mask) == 0);

  if (size <= sp->free_space) {
    size_t addr = sp->capacity - sp->free_space;
    uintptr_t unaligned_ptr = (uintptr_t) sp->items + addr;
    unsigned int misalignment = unaligned_ptr & alignment_mask;

    if (!misalignment) {
      void *item_ptr = (void *) unaligned_ptr;
      sp->free_space -= size;
      return item_ptr;
    } else {
      unsigned int align_extra = alignment - misalignment;
      if (size + align_extra <= sp->free_space) {
        void *item_ptr = (void *) (unaligned_ptr + align_extra);
        sp->free_space -= (size + align_extra);
        return item_ptr;
      }
    }
  }

  return NULL;
}

static void My_Scan_Frags(BCAllocPool *pool)
{
  size_t frag1_size = 0, frag2_size = 0;
  BCAllocSubPool *sub_pool1 = NULL, *sp;

  for (sp = pool->sub_pool.next; sp; sp = sp->next) {
    size_t fs = sp->free_space;
    if (fs > frag1_size) {
      sub_pool1 = sp;
      frag2_size = frag1_size;
      frag1_size = fs;
    } else if (fs > frag2_size)
      frag2_size = fs;
  }

  pool->sub_pool1 = sub_pool1;
  pool->frag1_size = frag1_size;
  pool->frag2_size = frag2_size;
}

void *BCAllocPool_Alloc_Aligned(BCAllocPool *pool, uint32_t size,
                                unsigned int alignment)
{
  BCAllocSubPool *sp, *new_sp;
  void *item_ptr, *new_items;
  size_t new_capacity;

  if (size == 0)
    return NULL;

  /* First try to use the free fragments. */
  if (pool->sub_pool1 && size <= pool->frag1_size) {
    item_ptr = My_Try_Alloc(pool->sub_pool1, size, alignment);

    if (item_ptr) {
      size_t frag1_new_size = pool->sub_pool1->free_space;
      if (frag1_new_size >= pool->frag2_size) {
        pool->frag1_size = frag1_new_size;
        return item_ptr;
      }

      /* Update fragment information in pool. */
      My_Scan_Frags(pool);
      return item_ptr;
    }
  }

  /* No fragments: use the active pool. */
  sp = & pool->sub_pool;
  item_ptr = My_Try_Alloc(sp, size, alignment);

  if (item_ptr)
    return item_ptr;

  /* Active pool is full: create one large enough to contain size bytes. */
  new_capacity = sp->capacity << 1;

  assert(new_capacity > sp->capacity);

  while (new_capacity < size + alignment)
    new_capacity <<= 1;

  new_items = Box_Mem_Alloc(new_capacity);
  new_sp = Box_Mem_Alloc(sizeof(BCAllocSubPool));
  if (!(new_sp && new_items)) {
    Box_Mem_Free(new_sp);
    Box_Mem_Free(new_items);
    return NULL;
  }

  *new_sp = *sp;
  sp->next = new_sp;
  sp->items = new_items;
  sp->capacity = new_capacity;
  sp->free_space = new_capacity;

  /* Update fragment information. */
  if (new_sp->free_space > pool->frag1_size) {
    pool->sub_pool1 = new_sp;
    pool->frag2_size = pool->frag1_size;
    pool->frag1_size = new_sp->free_space;
  } else if (new_sp->free_space > pool->frag2_size)
    pool->frag2_size = new_sp->free_space;

  return My_Try_Alloc(sp, size, alignment);
}

void BCAllocPool_Print_Stats(BCAllocPool *pool, FILE *out)
{
  int i;
  BCAllocSubPool *sp;
  fprintf(out, "Pool{frag1_size=%zu, frag2_size=%zu, sub_pool1=%p}\n",
          pool->frag1_size, pool->frag2_size, pool->sub_pool1);
  for (sp = & pool->sub_pool, i = 0; sp; sp = sp->next, i++) {
    fprintf(out, "%d: {free_space/capacity=%zu/%zu}\n",
            i, sp->free_space, sp->capacity);
  }
}

#ifdef TEST_ALLOCPOOL

int main(void) {
  int i;
  uint32_t szs[] = {1, 10, 1, 1, 1, 1, 1, 1, 1, 1};
  BCAllocPool *p = BCAllocPool_Create(sizeof(int));



  for (i = 0; i < sizeof(szs)/sizeof(szs[0]); i++) {
    void *ptr = BCAllocPool_Alloc_Aligned(p, szs[i], 1);
    printf("Allocated %d bytes in %p\n", szs[i], ptr);
    BCAllocPool_Print_Stats(p, stdout);
  }

  BCAllocPool_Destroy(p);

  exit(EXIT_SUCCESS);
}

#endif
