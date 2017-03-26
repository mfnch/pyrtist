/* registers.h - Autore: Franchin Matteo - agosto 2004
 *
 * Questo file contiene le dichiarazioni delle procedure definite in register.c
 */

#define REG_OCC_TYP_SIZE {10, 10, 10, 10, 10}
#define VAR_OCC_TYP_SIZE {10, 10, 10, 10, 10}

Task Reg_Init(UInt typical_num_reg[NUM_TYPES]);
Intg Reg_Occupy(Intg t);
Task Reg_Release(Intg t, UInt regnum);
Intg Reg_Num(Intg t);
Task Var_Init(UInt typical_num_var[NUM_TYPES]);
Intg Var_Occupy(Intg type, Intg level);
Task Var_Release(Intg type, UInt varnum);
Intg Var_Num(Intg type);
void RegVar_Get_Nums(Intg *num_var, Intg *num_reg);
