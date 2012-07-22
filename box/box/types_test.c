#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <box/types.h>
#include <box/ntypes.h>

static BoxType t_char, t_int, t_real;

#define MY_CREATE_INTRINSIC(t, Type, name)                              \
  do {                                                                  \
  BoxType it =                                                          \
    BoxType_Create_Intrinsic(sizeof(Type), __alignof__(Type));          \
  (t) = BoxType_Create_Ident(it, (name));                               \
  } while(0)

static void My_Init(void) {
  MY_CREATE_INTRINSIC(t_char, BoxChar, "Char");
  MY_CREATE_INTRINSIC(t_int, BoxInt, "Int");
  MY_CREATE_INTRINSIC(t_real, BoxReal, "Real");
  assert(t_char && t_int && t_real);
}

static void My_Finish(void) {
  BoxType_Unlink(t_char);
  BoxType_Unlink(t_int);
  BoxType_Unlink(t_real);
}

static BoxType
My_Create_Tuple_Structure(char *name1, BoxType memb1,
                          char *name2, BoxType memb2) {
  BoxType s = BoxType_Create_Structure();
  BoxType_Add_Member_To_Structure(s, memb1, name1);
  BoxType_Add_Member_To_Structure(s, memb2, name2);
  return s;
}

/* Test empty structures */
static BoxBool My_Test_Struct_Empty(void) {
  BoxBool result = BOXBOOL_TRUE;
  BoxTypeIter ti;
  BoxType t;

  BoxType s = BoxType_Create_Structure();
  if (BoxType_Get_Structure_Num_Members(s) != 0)
    return BOXBOOL_FALSE;

  BoxTypeIter_Init(& ti, s);
  if (BoxTypeIter_Get_Next(& ti, & t))
    result = BOXBOOL_FALSE;

  BoxType_Unlink(s);
  return result;
}

static BoxBool
My_Test_2Memb_Anonymous_Structure(void) {

  BoxType as = My_Create_Tuple_Structure(NULL, BoxType_Link(t_char),
                                         NULL, BoxType_Link(t_real));

  BoxType_Unlink(as);
  return BOXBOOL_TRUE;
}

static BoxBool
My_Test_Compare_Structure(void) {
  BoxType s1 = My_Create_Tuple_Structure("s11", BoxType_Link(t_char),
                                         "s12", BoxType_Link(t_real));
  BoxType s2 = My_Create_Tuple_Structure("s21", BoxType_Link(t_char),
                                         "s22", BoxType_Link(t_real));
  BoxBool result = (s1
                    && (BoxType_Compare(s1, s2) == BOXTYPECMP_EQUAL)
                    && (BoxType_Compare(s1, t_char) == BOXTYPECMP_DIFFERENT));
  BoxType_Unlink(s1);
  BoxType_Unlink(s2);
  return result;
}

static BoxBool My_Test_Combinations(void) {
  return BOXBOOL_FALSE;
}

#if 0
static void My_Print_Struct(BoxType s) {
  BoxTypeIter ti;
  BoxType t;
  size_t num_items = BoxType_Get_Structure_Num_Members(s);

  printf("Type %p: class=structure, num_items=%d\n", s, (int) num_items);
  for (BoxTypeIter_Init(& ti, s); BoxTypeIter_Get_Next(& ti, & t);) {
    char *member_name;
    size_t offset, size;
    BoxType type;

    BoxType_Get_Structure_Member(t, & member_name, & offset, & size, & type);
    printf("'%s': offset=%d, size=%d, type=%p\n",
           member_name, (int) offset, (int) size, type);
  }
}
#endif

int main(void) {
  int exit_status = EXIT_SUCCESS;

  struct {
    const char *test_name;
    BoxBool (*test_exec)(void);
  } *row, table[] = {
    {"empty structure", My_Test_Struct_Empty},
    {"compare 2 member structure", My_Test_Compare_Structure},
    {"anonymous 2 member structure", My_Test_2Memb_Anonymous_Structure},
    {"creating combinations", My_Test_Combinations},
    {NULL, NULL}
  };

  My_Init();

  for (row = & table[0]; row->test_exec; row++) {
    BoxBool result = row->test_exec();
    printf("Testing \"%s\": %s\n",
           row->test_name, result ? "success" : "failure");
    if (!result)
      exit_status = EXIT_FAILURE;
  }

  My_Finish();

  exit(exit_status);
}
