#ifndef lopcodes_h
#define lopcodes_h

#include "llimits.h"
#include "lobfuscator.h"

typedef enum {
  OP_MOVE, OP_LOADK, OP_LOADKX, OP_LOADBOOL, OP_LOADNIL, OP_GETUPVAL,
  OP_GETTABUP, OP_GETTABLE, OP_SETTABUP, OP_SETUPVAL, OP_SETTABLE, OP_NEWTABLE,
  OP_SELF, OP_ADD, OP_SUB, OP_MUL, OP_MOD, OP_POW, OP_DIV, OP_IDIV,
  OP_BAND, OP_BOR, OP_BXOR, OP_SHL, OP_SHR, OP_UNM, OP_BNOT, OP_NOT,
  OP_LEN, OP_CONCAT, OP_JMP, OP_EQ, OP_LT, OP_LE, OP_TEST, OP_TESTSET,
  OP_CALL, OP_TAILCALL, OP_RETURN, OP_FORLOOP, OP_FORPREP, OP_TFORCALL,
  OP_TFORLOOP, OP_SETLIST, OP_CLOSURE, OP_VARARG, OP_EXTRAARG, OP_TBC,
  OP_NEWARRAY, OP_TFOREACH, OP_VIRTUAL
} OpCode;

#define NUM_OPCODES	(cast(int, OP_VIRTUAL) + 1)

#define POS_OP		0
#define SIZE_OP		6
#define POS_A		6
#define SIZE_A		8
#define POS_C		14
#define SIZE_C		9
#define POS_B		23
#define SIZE_B		9
#define POS_Bx		14
#define SIZE_Bx		18
#define POS_Ax		6
#define SIZE_Ax		26

#define MASK1(n,p)	((~((~(Instruction)0)<<(n)))<<(p))
#define MASK0(n,p)	(~MASK1(n,p))

#define getarg(i,pos,size)	(cast(int, ((i)>>pos) & MASK1(size,0)))
#define setarg(i,v,pos,size)	((i) = (((i)&MASK0(size,pos)) | ((cast(Instruction, v)<<pos)&MASK1(size,pos))))

/* Raw macros for internal use */
#define GET_OPCODE(i)	(cast(OpCode, getarg(i, POS_OP, SIZE_OP)))
#define SET_OPCODE(i,o)	setarg(i, o, POS_OP, SIZE_OP)
#define GETARG_A(i)	getarg(i, POS_A, SIZE_A)
#define GETARG_B(i)	getarg(i, POS_B, SIZE_B)
#define GETARG_C(i)	getarg(i, POS_C, SIZE_C)
#define GETARG_Bx(i)	getarg(i, POS_Bx, SIZE_Bx)
#define GETARG_Ax(i)	getarg(i, POS_Ax, SIZE_Ax)
#define GETARG_sBx(i)	(GETARG_Bx(i)-MAXARG_sBx)
#define SETARG_A(i,v)	setarg(i, v, POS_A, SIZE_A)
#define SETARG_B(i,v)	setarg(i, v, POS_B, SIZE_B)
#define SETARG_C(i,v)	setarg(i, v, POS_C, SIZE_C)
#define SETARG_Bx(i,v)	setarg(i, v, POS_Bx, SIZE_Bx)
#define SETARG_Ax(i,v)	setarg(i, v, POS_Ax, SIZE_Ax)
#define SETARG_sBx(i,b)	SETARG_Bx((i),cast(unsigned int, (b)+MAXARG_sBx))

#define CREATE_ABC(o,a,b,c)	((cast(Instruction, o)<<POS_OP) 			| (cast(Instruction, a)<<POS_A) 			| (cast(Instruction, b)<<POS_B) 			| (cast(Instruction, c)<<POS_C))

#define CREATE_ABx(o,a,bc)	((cast(Instruction, o)<<POS_OP) 			| (cast(Instruction, a)<<POS_A) 			| (cast(Instruction, bc)<<POS_Bx))

#define CREATE_Ax(o,a)		((cast(Instruction, o)<<POS_OP) 			| (cast(Instruction, a)<<POS_Ax))

#define MAXARG_A        ((1<<SIZE_A)-1)
#define MAXARG_B        ((1<<SIZE_B)-1)
#define MAXARG_C        ((1<<SIZE_C)-1)
#define MAXARG_Bx       ((1<<SIZE_Bx)-1)
#define MAXARG_sBx      (MAXARG_Bx>>1)
#define MAXARG_Ax       ((1<<SIZE_Ax)-1)

#define BITRK		(1 << (SIZE_B - 1))
#define ISK(x)		((x) & BITRK)
#define INDEXK(r)	((int)(r) & ~BITRK)
#define MAXINDEXRK	(BITRK - 1)
#define RKASK(x)	((x) | BITRK)
#define NO_REG		MAXARG_A

enum OpArgMask { OpArgN, OpArgU, OpArgR, OpArgK };
enum OpMode {iABC, iABx, iAsBx, iAx};

LUAI_DDEC const lu_byte luaP_opmodes[NUM_OPCODES];
LUAI_DDEC const lu_byte luaP_op_encode[64];
LUAI_DDEC const lu_byte luaP_op_decode[64];

#define getOpMode(m)	(cast(enum OpMode, luaP_opmodes[m] & 3))
#define getBMode(m)	(cast(enum OpArgMask, (luaP_opmodes[m] >> 4) & 3))
#define getCMode(m)	(cast(enum OpArgMask, (luaP_opmodes[m] >> 2) & 3))
#define testAMode(m)	(luaP_opmodes[m] & (1 << 6))
#define testTMode(m)	(luaP_opmodes[m] & (1 << 7))

LUAI_DDEC const char *const luaP_opnames[NUM_OPCODES+1];
#define LFIELDS_PER_FLUSH	50

#endif
