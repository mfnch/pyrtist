/* List of all node types in Box Abstract Syntax Tree. */

/* Node used to represent immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(IMM, Imm)
#else
typedef struct BoxASTNodeImm_struct {
  BoxASTImm imm;
  BoxASTImmType type;
} BoxASTNodeImm;
#endif
