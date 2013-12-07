/* List of all node types in Box Abstract Syntax Tree. */

/* Node used to represent character immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(CHAR_IMM, CharImm)
#else
typedef struct BoxASTNodeCharImm_struct {
  BOXASTNODEHEAD
  BoxChar             value;
} BoxASTNodeCharImm;
#endif

/* Node used to represent integer immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(INT_IMM, IntImm)
#else
typedef struct BoxASTNodeIntImm_struct {
  BOXASTNODEHEAD
  BoxInt              value;
} BoxASTNodeIntImm;
#endif

/* Node used to represent real immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(REAL_IMM, RealImm)
#else
typedef struct BoxASTNodeRealImm_struct {
  BOXASTNODEHEAD
  BoxReal             value;
} BoxASTNodeRealImm;
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

/* Node used to represent boxes. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(BOX, Box)
#else
typedef struct BoxASTNodeBox_struct {
  BOXASTNODEHEAD
  BoxASTNode          *parent;
  BoxASTNodeStatement *first_stmt;
} BoxASTNodeBox;
#endif
