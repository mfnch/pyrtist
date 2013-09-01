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
BCAllocPool *BCAllocPool_Create(size_t item_size,
                                size_t initial_capacity)
{
  BCAllocPool *pool = Box_Mem_Alloc(sizeof(BCAllocPool));
  if (pool)
    BCAllocPool_Init(pool, item_size, initial_capacity);
  return pool;
}

/* Destroy the given allocation pool. */
void BCAllocPool_Destroy(BCAllocPool *pool)
{
  BCAllocPool_Finish(pool);
  Box_Mem_Free(pool);
}

/* Initialise an allocation pool. */
BCAllocPool *BCAllocPool_Init(BCAllocPool *pool, size_t item_size,
                              size_t initial_capacity)
{
  BCAllocSubPool *sp = & pool->sub_pool;
  void *items = Box_Mem_Alloc(initial_capacity*item_size);
  if (!items)
    return NULL;

  assert(item_size > 0 && initial_capacity > 0);

  sp->next = NULL;
  sp->items = items;
  sp->capacity = initial_capacity;
  sp->num_items = 0;
  pool->num_items = 0;
  pool->item_size = item_size;
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

void *BCAllocPool_Alloc(BCAllocPool *pool)
{
  BCAllocSubPool *sp = & pool->sub_pool;
  if (sp->num_items < sp->capacity) {
    size_t addr = sp->num_items * pool->item_size;
    void *item_ptr = (uint8_t *) sp->items + addr;
    (void) memset(item_ptr, 0, pool->item_size);
    sp->num_items++;
    pool->num_items++;
    return item_ptr;

  } else {
    BCAllocSubPool *new_sp;
    void *new_items;
    size_t new_capacity = sp->capacity << 1;
    assert(new_capacity > sp->capacity);

    new_items = Box_Mem_Alloc(new_capacity*pool->item_size);
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
    sp->num_items = 1;
    pool->num_items++;
    return sp->items;
  }
}

void BCAllocPool_Print_Stats(BCAllocPool *pool, FILE *out)
{
  int i;
  BCAllocSubPool *sp;
  fprintf(out, "Pool {num_items=%zu, item_size=%zu}\n",
          pool->num_items, pool->item_size);
  for (sp = & pool->sub_pool, i = 0; sp; sp = sp->next, i++) {
    fprintf(out, "%d: {num_items/capacity=%zu/%zu}\n",
            i, sp->num_items, sp->capacity);
  }
}

#ifdef TEST_ALLOCPOOL

int main(void) {
  int i;
  BCAllocPool *p = BCAllocPool_Create(sizeof(int), 1);

  for (i = 0; i < 10; i++) {
    void *ptr = BCAllocPool_Alloc(p);
    printf("Allocated %p\n", ptr);
    BCAllocPool_Print_Stats(p, stdout);
  }

  BCAllocPool_Destroy(p);

  exit(EXIT_SUCCESS);
}

#endif
