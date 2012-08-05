#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <box/types.h>
#include <box/ntypes.h>
#include <box/obj.h>
#include <box/core.h>


static BoxType t_char, t_int, t_real, t_point;

#define MY_CREATE_INTRINSIC(t, Type, name)                              \
  do {                                                                  \
  BoxType it =                                                          \
    BoxType_Create_Intrinsic(sizeof(Type), __alignof__(Type));          \
  (t) = BoxType_Create_Ident(it, (name));                               \
  } while(0)

static BoxBool My_Init(void) {
  if (!Box_Initialize_Type_System())
    return BOXBOOL_FALSE;

  MY_CREATE_INTRINSIC(t_char, BoxChar, "XChar");
  MY_CREATE_INTRINSIC(t_int, BoxInt, "XInt");
  MY_CREATE_INTRINSIC(t_real, BoxReal, "XReal");
  MY_CREATE_INTRINSIC(t_point, BoxPoint, "XPoint");

  return (t_char && t_int && t_real && t_point);
}

static void My_Finish(void) {
  BoxSPtr_Unlink(t_char);
  BoxSPtr_Unlink(t_int);
  BoxSPtr_Unlink(t_real);
  BoxSPtr_Unlink(t_point);

  Box_Finalize_Type_System();
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

  BoxSPtr_Unlink(s);
  return result;
}

static BoxBool
My_Test_2Memb_Anonymous_Structure(void) {

  BoxType as = My_Create_Tuple_Structure(NULL, BoxType_Link(t_char),
                                         NULL, BoxType_Link(t_real));

  BoxSPtr_Unlink(as);
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
  BoxSPtr_Unlink(s1);
  BoxSPtr_Unlink(s2);
  return result;
}

static BoxBool My_Test_Create_Combinations(void) {
  BoxBool
    r1 = BoxType_Define_Combination(t_int, BOXCOMBTYPE_AT, t_real, NULL),
    r2 = BoxType_Define_Combination(t_int, BOXCOMBTYPE_AT, t_char, NULL);
  return r1 && r2;
}

static BoxBool My_Test_Find_Combinations(void) {
  BoxTypeCmp e1, e2, e3, e4, e5;
  BoxType t1, t2, t3, t4, t5;

  if (!BoxType_Define_Combination(t_int, BOXCOMBTYPE_AT, t_real, NULL)
      || !BoxType_Define_Combination(t_int, BOXCOMBTYPE_AT, t_char, NULL))
    return BOXBOOL_FALSE;

  t1 = BoxType_Find_Combination(t_int, BOXCOMBTYPE_AT, t_real, & e1);
  t2 = BoxType_Find_Combination(t_int, BOXCOMBTYPE_COPY, t_real, & e2);
  t3 = BoxType_Find_Combination(t_int, BOXCOMBTYPE_AT, t_char, & e3);
  t4 = BoxType_Find_Combination(t_int, BOXCOMBTYPE_AT, t_int, & e4);
  t5 = BoxType_Find_Combination(t_char, BOXCOMBTYPE_AT, t_int, & e5);

  if (!(t1 && !t2 && t3 && !t4 && !t5))
    return BOXBOOL_FALSE;

  if (e1 != BOXTYPECMP_SAME || e3 != BOXTYPECMP_SAME)
    return BOXBOOL_FALSE;

  return BOXBOOL_TRUE;
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
    {"creating combinations", My_Test_Create_Combinations},
    {"finding combinations", My_Test_Find_Combinations},
    {NULL, NULL}
  };

  if (!My_Init())
    exit(EXIT_FAILURE);

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
