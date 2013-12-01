/* List of all node types in Box Abstract Syntax Tree. */

/* Node used to represent immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(IMM, Imm)
#else
typedef struct BoxASTNodeImm_struct {
  BOXASTNODEHEAD
  BoxASTImm imm;
  uint8_t   type;
} BoxASTNodeImm;
#endif

/* Node used to represent statement lists. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(STATEMENT, Statement)
#else
typedef struct BoxASTNodeStatement_struct BoxASTNodeStatement;
struct BoxASTNodeStatement_struct {
  BOXASTNODEHEAD
  BoxASTNodeStatement *next;
  BoxASTNode          *value;
  uint8_t             sep;
};
#endif
