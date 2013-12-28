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

/* Node used to represent string immediates. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(STR_IMM, StrImm)
#else
typedef struct BoxASTNodeStrImm_struct {
  BOXASTNODEHEAD
  char                *str;
} BoxASTNodeStrImm;
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

/* Node used to represent type identifiers. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(TYPE_IDFR, TypeIdfr)
#else
typedef struct BoxASTNodeTypeIdfr_struct {
  BOXASTNODEHEAD
  char                name[];
} BoxASTNodeTypeIdfr;
#endif

/* Node used to represent variable identifiers. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(VAR_IDFR, VarIdfr)
#else
typedef struct BoxASTNodeVarIdfr_struct {
  BOXASTNODEHEAD
  char                name[];
} BoxASTNodeVarIdfr;
#endif

/* Node used to mark an expression as ignorable. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(IGNORE, Ignore)
#else
typedef struct BoxASTNodeIgnore_struct {
  BOXASTNODEHEAD
  BoxASTNode          *value;
} BoxASTNodeIgnore;
#endif

/* Node used to mark an expression as ignorable. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(RAISE, Raise)
#else
typedef struct BoxASTNodeRaise_struct {
  BOXASTNODEHEAD
  BoxASTNode          *value;
} BoxASTNodeRaise;
#endif

/* Node used to represent an unary operation. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(UN_OP, UnOp)
#else
typedef struct BoxASTNodeUnOp_struct {
  BOXASTNODEHEAD
  BoxASTNode          *value;
  BoxASTUnOp          op;
} BoxASTNodeUnOp;
#endif

/* Node used to represent a binary operation. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(BIN_OP, BinOp)
#else
typedef struct BoxASTNodeBinOp_struct {
  BOXASTNODEHEAD
  BoxASTNode          *lhs, *rhs;
  BoxASTBinOp         op;
} BoxASTNodeBinOp;
#endif

/* Compound member. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(MEMBER, Member)
#else
typedef struct BoxASTNodeMember_struct BoxASTNodeMember;
struct BoxASTNodeMember_struct {
  BOXASTNODEHEAD
  BoxASTNode          *expr, *name;
  BoxASTNodeMember    *next;
};
#endif

/* Node used to represent a simple-parenthesis/structure/species. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(COMPOUND, Compound)
#else
typedef struct BoxASTNodeCompound_struct {
  BOXASTNODEHEAD
  BoxASTNodeMember    *memb;
  BoxSrc              sep_src;
  uint8_t             sep;
  uint8_t             kind;
} BoxASTNodeCompound;
#endif
