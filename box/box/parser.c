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
  FILE          *owned_file;
  char          *script_dir;
  BoxSrcFullPos full_pos;
} MyIncludeData;


BoxParser *BoxParser_Create(BoxPaths *paths, BoxLogger *logger)
{
  BoxParser *bp;

  bp = Box_Mem_Alloc(sizeof(BoxParser));
  if (!bp)
    return NULL;

  yylex_init(& bp->scanner);
  yyset_extra(bp, bp->scanner);
  bp->ast = BoxAST_Create(logger);

  bp->comment_level = 0;
  bp->paths = paths;
  bp->src.begin = bp->src.end = 0;
  BoxSrcFullPos_Init(& bp->full_pos, 0);

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
    BoxAST_Log(bl->ast, & bl->src, BOXLOGLEVEL_ERROR,
               "\"%s\" <-- Cannot find the file!", fn);
    return BOXBOOL_FALSE;
  }

  f = fopen(full_path, "rt");
  if (!f) {
    BoxAST_Log(bl->ast, & bl->src, BOXLOGLEVEL_ERROR,
               "\"%s\" <-- Cannot open the file!", fn);
    return BOXBOOL_FALSE;
  }

  if (BoxParser_Begin_Include_FILE(bl, fn, f, /*do_fclose*/BOXBOOL_TRUE)) {
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

int BoxParser_Get_Next_Token(BoxParser *bl)
{
  if (BoxArr_Num_Items(& bl->include_list) > 0)
    return yylex(bl->scanner);
  BoxAST_Log(bl->ast, NULL, BOXLOGLEVEL_ERROR,
             "BoxParser_Next_Token: the source file has not been "
             "specified. Use BoxParser_Begin_Include to set it.");
  abort();
}

BoxBool
BoxParser_Begin_Include_FILE(BoxParser *bl, const char *fn, FILE *f,
                             BoxBool do_fclose)
{
  size_t include_level = BoxArr_Num_Items(& bl->include_list);
  YY_BUFFER_STATE buffer;
  MyIncludeData *cur;

  if (include_level >= bl->max_include_level) {
    BoxAST_Log(bl->ast, & bl->src, BOXLOGLEVEL_ERROR,
               "Cannot include \"%s\": too many files included.", fn);
    return BOXBOOL_FALSE;
  }

  cur = BoxArr_Push(& bl->include_list, NULL);
  cur->owned_file = (do_fclose) ? f : NULL;
  cur->full_pos = bl->full_pos;
  cur->script_dir = NULL;

  buffer = yy_create_buffer(f, YY_BUF_SIZE, bl->scanner);
  assert(buffer != NULL);
  yypush_buffer_state(buffer, bl->scanner);

  bl->src.begin = ++bl->src.end;
  bl->full_pos.file_num = BoxAST_Get_File_Num(bl->ast, fn);
  bl->full_pos.line = bl->full_pos.col = 0;
  BoxAST_Store_Src_Map(bl->ast, bl->src.begin, & bl->full_pos);
  return BOXBOOL_TRUE;
}

BoxBool BoxParser_End_Include(BoxParser *bl)
{
  size_t include_level = BoxArr_Num_Items(& bl->include_list);
  if (include_level > 0) {
    MyIncludeData cur;

    /* Retrieve the last include data block and remove it from the stack. */
    BoxArr_Pop(& bl->include_list, & cur);

    /* Return to the previous buffer and restore the previous position. */
    yypop_buffer_state(bl->scanner);
    bl->src.begin = ++bl->src.end;
    bl->full_pos = cur.full_pos;
    BoxAST_Store_Src_Map(bl->ast, bl->src.begin, & bl->full_pos);
    Box_Mem_Free(cur.script_dir);
    if (cur.owned_file)
      fclose(cur.owned_file);
    return (include_level == 1);
  }

  return BOXBOOL_TRUE;
}
