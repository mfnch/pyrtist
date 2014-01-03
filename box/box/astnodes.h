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

/* Node used to express a member access. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(GET, Get)
#else
typedef struct BoxASTNodeGet_struct {
  BOXASTNODEHEAD
  BoxASTNode          *parent;
  BoxASTNodeVarIdfr   *name;
} BoxASTNodeGet;
#endif

/* Node used to represent a combination definition. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(COMB_DEF, CombDef)
#else
typedef struct BoxASTNodeCombDef_struct {
  BOXASTNODEHEAD
  BoxASTNode          *child, *parent, *implem, *c_name;
  uint8_t             comb_type;
} BoxASTNodeCombDef;
#endif

/* Node used to represent an argument in a combination body. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(ARG_GET, ArgGet)
#else
typedef struct BoxASTNodeArgGet_struct {
  BOXASTNODEHEAD
  uint32_t            depth;
} BoxASTNodeArgGet;
#endif

/* Node used to represent an argument in a combination body. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(TYPE_TAG, TypeTag)
#else
typedef struct BoxASTNodeTypeTag_struct {
  BOXASTNODEHEAD
  uint32_t            type_id;
} BoxASTNodeTypeTag;
#endif

/* Node used to represent a subtype. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(SUBTYPE, Subtype)
#else
typedef struct BoxASTNodeSubtype_struct {
  BOXASTNODEHEAD
  BoxASTNode          *parent;
  BoxASTNodeTypeIdfr  *name;
} BoxASTNodeSubtype;
#endif

/* Node used to represent a keyword. */
#ifdef BOXASTNODE_DEF
BOXASTNODE_DEF(KEYWORD, Keyword)
#else
typedef struct BoxASTNodeKeyword_struct {
  BOXASTNODEHEAD
  BoxASTNodeTypeIdfr  *type;
} BoxASTNodeKeyword;
#endif
