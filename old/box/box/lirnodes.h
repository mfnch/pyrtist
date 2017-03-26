/* List of all LIR nodes. */

/* Node used to represent a generic 0-arguments instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP, Op)
#else
typedef struct BoxLIRNodeOp_struct BoxLIRNodeOp;

struct BoxLIRNodeOp_struct {
  BOXLIRNODEHEAD
  uint8_t           op_id;
  uint8_t           cats[2];
  int32_t           offset;
  BoxLIRNodeOp      *next;
};
#endif

/* Node used to represent a generic 1-argument instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP1, Op1)
#else
typedef struct BoxLIRNodeOp1_struct {
  BoxLIRNodeOp      op;
  int32_t           regs[1];
} BoxLIRNodeOp1;
#endif

/* Node used to represent a generic 2-arguments instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP2, Op2)
#else
typedef struct BoxLIRNodeOp2_struct {
  BoxLIRNodeOp      op;
  int32_t           regs[2];
} BoxLIRNodeOp2;
#endif

/* Node used to represent a char immediate load instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP_LD_CHAR, OpLdChar)
#else
typedef struct BoxLIRNodeOpLdChar_struct {
  BoxLIRNodeOp      op;
  int32_t           regs[1];
  char              value;
} BoxLIRNodeOpLdChar;
#endif

/* Node used to represent an int immediate load instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP_LD_INT, OpLdInt)
#else
typedef struct BoxLIRNodeOpLdInt_struct {
  BoxLIRNodeOp      op;
  int32_t           regs[1];
  BoxInt            value;
} BoxLIRNodeOpLdInt;
#endif

/* Node used to represent a real immediate load instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP_LD_REAL, OpLdReal)
#else
typedef struct BoxLIRNodeOpLdReal_struct {
  BoxLIRNodeOp      op;
  int32_t           regs[1];
  BoxReal           value;
} BoxLIRNodeOpLdReal;
#endif

/* Node used to represent a branch instruction. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP_BRANCH, OpBranch)
#else
typedef struct BoxLIRNodeOpBranch_struct {
  BoxLIRNodeOp      op,
                    *target;
} BoxLIRNodeOpBranch;
#endif

/* Node used to represent a branch target (label). */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(OP_LABEL, OpLabel)
#else
typedef struct BoxLIRNodeOpLabel_struct {
  BoxLIRNodeOp      op;
  BoxLIRNodeOp      *prev;
} BoxLIRNodeOpLabel;
#endif

/* Node used to represent a procedure. */
#ifdef BOXLIRNODE_DEF
BOXLIRNODE_DEF(PROC, Proc)
#else
typedef struct BoxLIRNodeProc_struct BoxLIRNodeProc;

struct BoxLIRNodeProc_struct {
  BoxLIRNodeProc    *next;
  BoxLIRNodeOp      *first_op,
                    *last_op;
};
#endif
