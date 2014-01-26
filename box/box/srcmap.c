/****************************************************************************
 * Copyright (C) 2014 by Matteo Franchin                                    *
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

#include <assert.h>
#include <box/mem.h>
#include <box/srcmap.h>

/**
 * @brief Size of the mapping table in each leaf.
 */
#define MY_NL_SIZE 112

/**
 * @brief Leaf of the binary search tree.
 *
 * Each leaf contains a table which contains several correspondences linear-
 * position -> full position. A leaf is searched linearly, but this is fine, as
 * every call to BoxSrcMap_Map() requires only one such search. At the same
 * time, the leaf contains compressed data when possible, which gives good
 * memory performance.
 */
typedef struct {
  uint32_t      lin_pos;               /**< Linear position. */
  BoxSrcFullPos full_pos;              /**< Full position. */
  uint8_t       new_lines[MY_NL_SIZE]; /**< Incremental newlines positions. */
} MyLeaf;

/**
 * @brief Node in the binary search tree.
 */
typedef struct {
  void      *parent, /**< Parent node, or @c NULL if this is the top node. */
            *left,   /**< Left node or left leaf. */
            *right;  /**< Right node or right leaf. */
  uint32_t  lin_pos; /**< Linear position used as key in the binary search. */
} MyNode;

/**
 * @brief Implementation of #BoxSrcMap.
 */
struct BoxSrcMap_struct {
  MyNode        *leftmost_node;  /**< Firstly (and leftmost) allocated node. */
  MyNode        *cur_node;       /**< Current node (for filling). */
  MyLeaf        *cur_leaf;       /**< Current leaf (for filling). */
  MyNode        *root;           /**< Root of the tree. */
  uint32_t      cur_leaf_length; /**< Length of the BoxSrcMap#cur_leaf. */
  uint32_t      cur_lin_pos;     /**< Linear position in BoxSrcMap#cur_leaf. */
  BoxSrcFullPos cur_full_pos;    /**< Current ``fill'' position. */
  int           depth;           /**< Depth of the tree (=0 for empty tree). */
};

/* Forward declarations. */
static BoxBool
My_Add_To_Leaf(BoxSrcMap *sm, uint32_t lin_pos, BoxSrcFullPos *fp);


/* Initialize a #BoxSrcMap object. */
void
BoxSrcMap_Init(BoxSrcMap *sm)
{
  sm->leftmost_node = NULL;
  sm->cur_node = NULL;
  sm->cur_leaf = NULL;
  sm->root = NULL;
  sm->cur_leaf_length = 0;
  sm->cur_lin_pos = 0;
  sm->cur_full_pos.file_num = 0;
  sm->cur_full_pos.line = 0;
  sm->cur_full_pos.col = 0;
  sm->depth = 0;
}

/* Finalize a #BoxSrcMap object initialized with BoxSrcMap_Init(). */
void
BoxSrcMap_Finish(BoxSrcMap *sm)
{
  MyNode *node, *parent_node;
  int depth = sm->depth, d;

  if (depth > 0) {
    /* Free the leftmost node. */
    parent_node = sm->leftmost_node->parent;
    Box_Mem_Free(sm->leftmost_node->left);
    Box_Mem_Free(sm->leftmost_node);

    for (d = depth - 1; d; d--) {
      int d1;
      MyNode *leaves_node;

      node = parent_node;
      parent_node = parent_node->parent;

      /* Free all the leaves in the right branch. */
      for (d1 = d, leaves_node = node->right; d1 < depth; d1++)
        leaves_node = leaves_node->left;
      Box_Mem_Free(leaves_node);

      /* Free this node and all the nodes in the right branch. */
      Box_Mem_Free(node);
    }
  }
}

/* Create a #BoxSrcMap object. */
BoxSrcMap *
BoxSrcMap_Create(void)
{
  BoxSrcMap *sm = (BoxSrcMap *) Box_Mem_Alloc(sizeof(BoxSrcMap));
  if (sm)
    BoxSrcMap_Init(sm);
  return sm;
}

/* Destroy a #BoxSrcMap object created with BoxSrcMap_Create(). */
void
BoxSrcMap_Destroy(BoxSrcMap *sm)
{
  if (sm)
    BoxSrcMap_Finish(sm);
  Box_Mem_Free(sm);
}

/* Grow the current tree by adding a new branch of the given depth to the
 * root. The branch is allocated all in one single malloc() call. This is done
 * so that the number of calls to malloc() is ~ log2(# leaves).
 */
static BoxBool
My_Grow_Tree(BoxSrcMap *sm, int depth)
{
  size_t num_leaves, num_nodes;
  MyLeaf *all_leaves;
  MyNode *all_nodes;
  int d;

  /* Linear positions are 32-bit, therefore depth < 31. */
  assert(depth >= 0 && depth < 32);

  if (depth)
    num_nodes = num_leaves = 1 << depth;
  else {
    num_nodes = 1;
    num_leaves = 2;
  }

  all_leaves = (MyLeaf *) Box_Mem_Alloc(sizeof(MyLeaf)*num_leaves);
  all_nodes = (MyNode *) Box_Mem_Alloc(sizeof(MyNode)*num_nodes);

  if (!(all_leaves && all_nodes)) {
    Box_Mem_Free(all_leaves);
    Box_Mem_Free(all_nodes);
    return BOXBOOL_FALSE;
  }

  if (depth == 0) {
    all_nodes[0].left = & all_leaves[0];
    all_nodes[0].right = & all_leaves[1];
    all_nodes[0].parent = NULL;
    sm->root = sm->leftmost_node = & all_nodes[0];
    sm->cur_node = sm->root;
    sm->cur_leaf = (MyLeaf *) sm->root->left;
    sm->cur_leaf_length = 0;
    sm->depth = 1;
    return BOXBOOL_TRUE;
  }

  all_nodes[0].left = sm->root;
  all_nodes[0].right = & all_nodes[1];
  all_nodes[0].parent = NULL;
  sm->root = & all_nodes[0];
  ((MyNode *) sm->root->left)->parent = sm->root;
  ((MyNode *) sm->root->right)->parent = sm->root;
  sm->depth++;

  /* In principle, the right nodes could be binary heaps, but this would make
   * the implementation more complicated without any relevant gains.
   */

  /* Link together the nodes. */
  for (d = 0; d < depth - 1; d++) {
    uint32_t p0_idx = 1 << d, plast_idx = p0_idx + p0_idx, p_idx;

    for (p_idx = p0_idx; p_idx < plast_idx; p_idx++) {
      uint32_t c_idx = p_idx + p_idx;
      MyNode *parent = & all_nodes[p_idx];
      parent->left = & all_nodes[c_idx];
      parent->right = & all_nodes[c_idx + 1];
      ((MyNode *) parent->left)->parent = parent;
      ((MyNode *) parent->right)->parent = parent;
    }
  }

  {
    uint32_t p0_idx = 1 << (depth - 1), p_idx;

    /* Now link the leaves. */
    for (p_idx = 0; p_idx < p0_idx; p_idx++) {
      uint32_t c_idx = p_idx + p_idx;
      MyNode *parent = & all_nodes[p0_idx + p_idx];
      parent->left = & all_leaves[c_idx];
      parent->right = & all_leaves[c_idx + 1];
    }

    /* ...and change the current leaf. */
    sm->cur_node = & all_nodes[p0_idx];
    sm->cur_leaf = (MyLeaf *) sm->cur_node->left;
    sm->cur_leaf_length = 0;
  }
  return BOXBOOL_TRUE;
}

/* Add a new leaf to the source map. */
static BoxBool
My_Use_New_Leaf(BoxSrcMap *sm)
{
  MyNode *parent_node;
  void *filled_node;
  int depth;

  /* Ascend the binary until we find a free right branch. */
  for (filled_node = sm->cur_leaf, parent_node = sm->cur_node, depth = 0;
       parent_node;
       filled_node = parent_node, parent_node = parent_node->parent, depth++) {
    if (filled_node == parent_node->left) {
      /* Found a free right node: now descend of the same depth. */
      MyNode *empty_node;
      for (empty_node = (MyNode *) parent_node->right; depth > 0; depth--) {
        parent_node = empty_node;
        empty_node = parent_node->left;
      }

      /* Move to the free leaf. */
      sm->cur_node = parent_node;
      sm->cur_leaf = (MyLeaf *) empty_node;
      sm->cur_leaf_length = 0;
      assert(sm->cur_leaf);
      return BOXBOOL_TRUE;
    }
  }

  /* The tree is full. We need to create a new branch. */
  return My_Grow_Tree(sm, depth);
}

/* Return the log2 of the size (in bytes) of a uint32_t integer. */
static int
My_Get_UInt32_Log2_Size(uint32_t in)
{
  return (in >= 0x100) + (in >= 0x10000);
}

/* Write an unsigned integer in a byte array. */
static uint8_t *
My_Write_UInt(uint8_t *output, uint32_t value, int size)
{
  switch (size) {
  case 0:
    return output;
  case 1:
    output[0] = (uint8_t) value;
    return output + 1;
  case 2:
    output[0] = value & 0xff; output[1] = (value >> 8) & 0xff;
    return output + 2;
  }
  output[0] = value & 0xff;
  output[1] = (value >> 8) & 0xff;
  output[2] = (value >> 16) & 0xff;
  output[3] = (value >> 24) & 0xff;
  return output + 4;
}

/* Read an unsigned integer from a byte array. */
static uint8_t *
My_Read_UInt(uint8_t *input, uint32_t *value, int size,
             uint32_t default_val)
{
  switch (size) {
  case 0:
    *value = default_val;
    return input;
  case 1:
    *value = (uint32_t) input[0];
    return input + 1;
  case 2:
    *value = (uint32_t) input[0] | ((uint32_t) input[1]) << 8;
    return input + 2;
  }
  *value = ((uint32_t) input[0] | ((uint32_t) input[1]) << 8
            | ((uint32_t) input[2]) << 16 | ((uint32_t) input[3]) << 24);
  return input + 4;
}

/* Add an uncompressed map entry to the current leaf. */
static BoxBool
My_Add_To_Leaf_Ext(BoxSrcMap *sm, uint32_t lin_pos, BoxSrcFullPos *fp)
{
  uint8_t extattr = 0;
  int entry_size = 2;
  uint8_t *output;
  uint32_t output_length = sm->cur_leaf_length;
  uint32_t delta_pos;

  if (output_length == 0)
    return My_Add_To_Leaf(sm, lin_pos, fp);
  --output_length;

  assert(lin_pos > sm->cur_lin_pos);
  delta_pos = lin_pos - sm->cur_lin_pos;
  if (delta_pos != 1) {
    int log2_sz = My_Get_UInt32_Log2_Size(delta_pos);
    entry_size += 1 << log2_sz;
    extattr = 1 + log2_sz;
  }

  if (fp->file_num != sm->cur_full_pos.file_num) {
    int log2_sz = My_Get_UInt32_Log2_Size(fp->file_num);
    entry_size += 1 << log2_sz;
    extattr = (1 + log2_sz) << 2;
  }

  if (fp->col != 0) {
    int log2_sz = My_Get_UInt32_Log2_Size(fp->col);
    entry_size += 1 << log2_sz;
    extattr |= (1 + log2_sz) << 4;
  }

  if (fp->line != sm->cur_full_pos.line + 1) {
    int log2_sz = My_Get_UInt32_Log2_Size(fp->line);
    entry_size += 1 << log2_sz;
    extattr |= (1 + log2_sz) << 6;
  }

  /* Check whether there is space to add the entry in the current leaf. */
  if (output_length + entry_size > MY_NL_SIZE) {
    /* No space: fill current leaf with zeros. */
    for (output = & sm->cur_leaf->new_lines[output_length];
         output_length < MY_NL_SIZE; output_length++)
      *(output++) = 0;

    /* Create a new leaf and try again. */
    if (!My_Use_New_Leaf(sm))
      return BOXBOOL_FALSE;

    assert(sm->cur_leaf_length == 0);
    return My_Add_To_Leaf(sm, lin_pos, fp);
  }

  /* Write uncompressed entry. */
  output = & sm->cur_leaf->new_lines[output_length];
  *(output++) = 0;
  *(output++) = extattr;
  if (extattr & 0x3)
    output = My_Write_UInt(output, delta_pos, extattr & 0x3);
  if (extattr & 0xc)
    output = My_Write_UInt(output, fp->file_num, (extattr & 0xc) >> 2);
  if (extattr & 0x30)
    output = My_Write_UInt(output, fp->col, (extattr & 0x30) >> 4);
  if (extattr & 0xc0)
    output = My_Write_UInt(output, fp->line, (extattr & 0xc0) >> 6);

  sm->cur_leaf_length += entry_size;
  sm->cur_lin_pos = lin_pos;
  sm->cur_full_pos = *fp;
  return BOXBOOL_TRUE;
}

/* Add a map entry to the current leaf, trying to compress it when possible. */
static BoxBool
My_Add_To_Leaf(BoxSrcMap *sm, uint32_t lin_pos, BoxSrcFullPos *fp)
{
  MyLeaf *cl = sm->cur_leaf;

  /* Deal in a special way with the first entry of the leaf. */
  if (sm->cur_leaf_length == 0) {
    int depth;
    MyNode *child, *parent;

    cl->lin_pos = lin_pos;
    cl->full_pos = *fp;
    sm->cur_leaf_length = 1;
    sm->cur_lin_pos = lin_pos;
    sm->cur_full_pos = *fp;

    /* Propagate the linear positions to the inner nodes.
     * This information will be used in the binary search.
     */
    child = (MyNode *) sm->cur_leaf;
    parent = sm->cur_node;
    for (depth = sm->depth; depth;
         depth--, child = parent, parent = parent->parent)
      if (child != parent->left) {
        parent->lin_pos = lin_pos;
        break;
      } else
        /* Set node key to very high value, so that left node is always
         * chosen in binary search.
         */
        parent->lin_pos = ~((uint32_t) 0);
    return BOXBOOL_TRUE;
  }

  /* Check whether we can use incremental positions. */
  if (fp->col == 0 && fp->line == sm->cur_full_pos.line + 1
      && fp->file_num == sm->cur_full_pos.file_num) {
    uint32_t delta_pos = lin_pos - sm->cur_lin_pos;
    assert(lin_pos > sm->cur_lin_pos);
    if (delta_pos < 256) {
      uint32_t nl_pos = sm->cur_leaf_length++ - 1;
      assert(nl_pos < MY_NL_SIZE);
      cl->new_lines[nl_pos] = (uint8_t) delta_pos;
      sm->cur_lin_pos = lin_pos;
      sm->cur_full_pos.line = fp->line;
      sm->cur_full_pos.col = 0;
      return BOXBOOL_TRUE;
    }
  }

  /* Cannot use incremental positions: write extended entry. */
  return My_Add_To_Leaf_Ext(sm, lin_pos, fp);
}

/* Map a linear position to the corresponding full position, assuming the
 * input linear position is inside the given leaf.
 */
static BoxBool
My_Find_In_Leaf(BoxSrcMap *sm, MyLeaf *leaf, int leaf_length,
                uint32_t lin_pos, BoxSrcFullPos *fp)
{
  uint32_t file_num, col, line, cur_lin_pos;
  uint8_t *input;

  if (leaf_length < 1 || lin_pos < leaf->lin_pos) {
    if (leaf != sm->leftmost_node->left)
      return BOXBOOL_FALSE;
    fp->file_num = leaf->full_pos.file_num;
    fp->line = 0;
    fp->col = lin_pos;
    return BOXBOOL_TRUE;
  }

  cur_lin_pos = leaf->lin_pos;
  file_num = leaf->full_pos.file_num;
  line = leaf->full_pos.line;
  col = leaf->full_pos.col;

  for (input = & leaf->new_lines[0]; leaf_length > 0;) {
    uint32_t delta_pos = (uint32_t) *(input++);
    leaf_length--;

    if (delta_pos) {
      uint32_t next_lin_pos = cur_lin_pos + delta_pos;
      if (lin_pos < next_lin_pos)
        break;
      cur_lin_pos = next_lin_pos;
      col = 0;
      line++;
    } else {
      if (leaf_length > 0) {
        uint8_t extattr = *(input++);
        int sz0 = extattr & 0x3, sz1 = (extattr & 0xc) >> 2,
          sz2 = (extattr & 0x30) >> 4, sz3 = (extattr & 0xc0) >> 6,
          entry_size = ((sz0 ? 1 << (sz0 - 1) : 0) +
                        (sz1 ? 1 << (sz1 - 1) : 0) +
                        (sz2 ? 1 << (sz2 - 1) : 0) +
                        (sz3 ? 1 << (sz3 - 1) : 0)) + 1;
        if (extattr && leaf_length >= entry_size) {
          input = My_Read_UInt(input, & delta_pos, sz0, 1);
          uint32_t next_lin_pos = cur_lin_pos + delta_pos;
          if (lin_pos < next_lin_pos)
            break;

          cur_lin_pos = next_lin_pos;
          input = My_Read_UInt(input, & file_num, sz1, file_num);
          input = My_Read_UInt(input, & col, sz2, 0);
          input = My_Read_UInt(input, & line, sz3, line + 1);
          leaf_length -= entry_size;
          continue;
        }
      }

      /* Got to the end of the list: quit the for loop. */
      break;
    }
  }

  assert(cur_lin_pos <= lin_pos);
  fp->file_num = file_num;
  fp->line = line;
  fp->col = lin_pos - cur_lin_pos + col;
  return BOXBOOL_TRUE;
}

BoxBool
BoxSrcMap_Store(BoxSrcMap *sm, uint32_t lin_pos,
                uint32_t file_num, uint32_t line, uint32_t col)
{
  BoxSrcFullPos fp;
  fp.file_num = file_num;
  fp.line = line;
  fp.col = col;
  return BoxSrcMap_Store_FP(sm, lin_pos, & fp);
}

BoxBool
BoxSrcMap_Store_FP(BoxSrcMap *sm, uint32_t lin_pos, BoxSrcFullPos *fp)
{
  if (sm->cur_leaf || My_Grow_Tree(sm, 0)) {
    if (sm->cur_leaf_length > MY_NL_SIZE) {
      /*sm->cur_leaf_length == MY_NL_SIZE + 1: leaf is full. */
      if (!My_Use_New_Leaf(sm))
        return BOXBOOL_FALSE;;
    }
    return My_Add_To_Leaf(sm, lin_pos, fp);
  }

  return BOXBOOL_FALSE;
}

BoxBool
BoxSrcMap_Map(BoxSrcMap *sm, uint32_t lin_pos,
              uint32_t *file_num, uint32_t *line, uint32_t *col)
{
  BoxSrcFullPos fp;
  if (!BoxSrcMap_Map_FP(sm, lin_pos, & fp))
    return BOXBOOL_FALSE;
  if (file_num)
    *file_num = fp.file_num;
  if (line)
    *line = fp.line;
  if (col)
    *col = fp.col;
  return BOXBOOL_TRUE;
}

BoxBool
BoxSrcMap_Map_FP(BoxSrcMap *sm, uint32_t pos, BoxSrcFullPos *fp)
{
  if (!fp)
    return BOXBOOL_FALSE;

  if (pos >= sm->cur_lin_pos) {
    fp->file_num = sm->cur_full_pos.file_num;
    fp->line = sm->cur_full_pos.line;
    fp->col = sm->cur_full_pos.col + (pos - sm->cur_lin_pos);
    return BOXBOOL_TRUE;
  } else {
    MyNode *node;
    MyLeaf *leaf;
    int leaf_length, depth;

    for (depth = sm->depth, node = sm->root; depth; depth--)
      node = (MyNode *) ((pos < node->lin_pos) ? node->left : node->right);

    leaf = (MyLeaf *) node;
    leaf_length = (leaf == sm->cur_leaf) ? sm->cur_leaf_length : MY_NL_SIZE;
    return My_Find_In_Leaf(sm, leaf, leaf_length, pos, fp);
  }
}

#ifdef TEST_SRCMAP
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

static void
My_Run_Test(uint32_t file_num, const char *content, size_t size)
{
  BoxSrcMap sm;
  uint32_t lin_pos, line, col, mfile_num, mline, mcol;
  int max_errs = 10;
  char c[2];
  c[1] = 0;

  assert(size = (uint32_t) size);

  BoxSrcMap_Init(& sm);

  /* First let's map the file. */
  for (lin_pos = 0, line = 0, col = 0; lin_pos < size; lin_pos++) {
    c[0] = content[lin_pos];
    if (c[0] == '\n') {
      line++;
      BoxSrcMap_Store(& sm, lin_pos + 1, file_num, line, 0);
    }
  }

  /* Now let's check the mapping. */
  for (lin_pos = 0, line = 0, col = 0; lin_pos < size; lin_pos++) {
    BoxBool success;
    c[0] = content[lin_pos];

    success = BoxSrcMap_Map(& sm, lin_pos, & mfile_num, & mline, & mcol);
    if (!success || mline != line || mcol != col || mfile_num != file_num) {
      if (max_errs > 0) {
        typedef unsigned int ui;
        max_errs--;
        if (success)
          fprintf(stderr, "Error: '%s' %u -> (%u, %u, %u) should be "
                  "(%u, %u, %u)\n", (c[0] == '\n' ? "\\n" : c), (ui) lin_pos,
                  (ui) mfile_num, (ui) mline, (ui) mcol,
                  (ui) file_num, (ui) line, (ui) col);
        else
          fprintf(stderr, "Cannot perform map at %u ~ (%u, %u, %u)\n",
                  (ui) lin_pos, (ui) file_num, (ui) line, (ui) col);
      } else if (max_errs == 0) {
        fprintf(stderr, "... Too many errors\n");
        max_errs--;
      }
    }

    if (c[0] != '\n')
      col++;
    else {
      line++;
      col = 0;
    }
  }

  BoxSrcMap_Finish(& sm);
}

int
main(int argc, const char **argv)
{
  FILE *f;
  const char *file_name;
  char *content;
  struct stat st;

  if (argc != 2) {
    fprintf(stderr, "Usage: %s file.txt\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  file_name = argv[1];
  if (stat(file_name, & st)) {
    fprintf(stderr, "Cannot stat file '%s'\n", file_name);
    exit(EXIT_FAILURE);
  }

  content = (char *) malloc(st.st_size);
  if (!content) {
    fprintf(stderr, "Cannot malloc(%zu)\n", st.st_size);
    exit(EXIT_FAILURE);
  }

  f = fopen(file_name, "r");
  if (!f) {
    fprintf(stderr, "Cannot open file '%s'\n", file_name);
    exit(EXIT_FAILURE);
  }

  if (fread(content, st.st_size, 1, f) != 1) {
    fprintf(stderr, "Error reading from file '%s'\n", file_name);
    exit(EXIT_FAILURE);
  }
  fclose(f);

  My_Run_Test(12, content, st.st_size);
  free(content);
  exit(EXIT_SUCCESS);
}
#endif