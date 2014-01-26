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

/** Data used to handle files included with "include". */
typedef struct {
  char          *script_dir;
  BoxSrcFullPos full_pos;
  BoxUInt       num_errs, num_warns;
} MyIncludeData;


BoxParser *BoxParser_Create(BoxPaths *paths)
{
  BoxParser *bp;

  bp = Box_Mem_Alloc(sizeof(BoxParser));
  if (!bp)
    return NULL;

  yylex_init(& bp->scanner);
  yyset_extra(bp, bp->scanner);
  bp->ast = BoxAST_Create(NULL);

  bp->comment_level = 0;
  bp->paths = paths;
  bp->src.begin = bp->src.end = 0;

  bp->max_include_level = TOK_MAX_INCLUDE;
  BoxArr_Init(& bp->include_list, sizeof(MyIncludeData),
              TOK_TYPICAL_MAX_INCLUDE);

  BoxHT_Init_Default(& bp->provided_features, 64);

  bp->parsing_macro = 0;
  BoxArr_Init(& bp->macro_content, 1, 64);

  if (!bp->ast) {
    BoxParser_Destroy(bp);
    return NULL;
  }

  return bp;
}

BoxAST *BoxParser_Destroy(BoxParser *bp)
{
  BoxAST *ast = bp->ast;
  yylex_destroy(bp->scanner);
  BoxHT_Finish(& bp->provided_features);
  BoxArr_Finish(& bp->include_list);
  BoxArr_Finish(& bp->macro_content);
  Box_Mem_Free(bp);
  return ast;
}

BoxBool BoxParser_Was_Provided(BoxParser *bl,
                               const char *feature, int length)
{
  BoxHTItem *item;
  if (BoxHT_Find(& bl->provided_features, (void *) feature, length, & item))
    return BOXBOOL_TRUE;
  BoxHT_Insert(& bl->provided_features, (void *) feature, length);
  return BOXBOOL_FALSE;
}

const char *BoxParser_Get_Script_Dir(BoxParser *bl)
{
  size_t num_included_files = BoxArr_Get_Num_Items(& bl->include_list);
  if (num_included_files > 0) {
    MyIncludeData *cur = BoxArr_Last_Item_Ptr(& bl->include_list);
    return cur->script_dir;
  }
  return NULL;
}

BoxBool BoxParser_Begin_Include(BoxParser *bl, const char *fn)
{
  const char *script_dir = BoxParser_Get_Script_Dir(bl);
  char *full_path = BoxPaths_Find_Inc_File(bl->paths, script_dir, fn);
  FILE *f = NULL;

  if (!full_path) {
    MSG_ERROR("\"%s\" <-- Cannot find the file!", fn);
    return BOXBOOL_FALSE;
  }

  f = fopen(full_path, "rt");
  if (!f) {
    MSG_ERROR("\"%s\" <-- Cannot open the file!", fn);
    return BOXBOOL_FALSE;
  }

  if (BoxParser_Begin_Include_FILE(bl, f, fn)) {
    MyIncludeData *cur = BoxArr_Get_Last_Item_Ptr(& bl->include_list);
    Box_Split_Path(& cur->script_dir, NULL, full_path);
    return BOXBOOL_TRUE;
  }

  return BOXBOOL_FALSE;
}

BoxAST *BoxParser_Get_AST(BoxParser *bp)
{
  return bp->ast;
}
