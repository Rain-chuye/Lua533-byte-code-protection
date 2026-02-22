/*
** $Id: llex.h,v 1.79 2016/03/02 17:56:43 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#ifndef llex_h
#define llex_h

#include "lobject.h"
#include "lzio.h"


#define FIRST_RESERVED	257


/*
* WARNING: if you change the order of this enumeration,
* grep "ORDER RESERVED"
*/
enum RESERVED {
  /* terminal symbols denoted by reserved words */
  TK_AND = FIRST_RESERVED, TK_BREAK, TK_CASE, TK_CONTINUE, TK_DEFAULT, TK_DEFER,
  TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
  TK_GOTO, TK_IF, TK_IN, TK_LAMBDA, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
  TK_RETURN, TK_SWITCH, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHEN, TK_WHILE,
  TK_TRY, TK_CATCH, TK_FINALLY, TK_ASYNC, TK_AWAIT, TK_NAMESPACE, TK_USING,
  TK_STRUCT, TK_ENUM, TK_ASM,
  TK_CLASS, TK_EXTENDS, TK_IMPLEMENTS, TK_PUBLIC, TK_PRIVATE, TK_PROTECTED,
  TK_STATIC, TK_NEW, TK_SUPER,
  TK_LUAVMP,
  /* other terminal symbols */
  TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
  TK_SHL, TK_SHR,
  TK_PLUSEQ, TK_MINUSEQ, TK_MULTEQ, TK_DIVEQ, TK_IDIVEQ, TK_MODEQ, TK_POWEQ,
  TK_CONCATEQ, TK_BITANDEQ, TK_BITOREQ, TK_BITXOREQ, TK_SHLEQ, TK_SHREQ,
  TK_PPLUS, TK_COALESCE, TK_OPTIONAL, TK_SPACESHIP, TK_PIPE, TK_BPIPE, TK_SPIPE,
  TK_DBCOLON, TK_EOS,
  TK_LET, TK_MEAN,
  TK_FLT, TK_INT, TK_NAME, TK_STRING
};

/* number of reserved words */
#define NUM_RESERVED	(cast(int, TK_LUAVMP-FIRST_RESERVED+1))


typedef union {
  lua_Number r;
  lua_Integer i;
  TString *ts;
} SemInfo;  /* semantics information */


typedef struct Token {
  int token;
  SemInfo seminfo;
} Token;


typedef struct LexState {
  int current;  /* current character (lookahead) */
  int linenumber;  /* input line counter */
  int lastline;  /* line of last token 'consumed' */
  Token t;  /* current token */
  Token lookahead;  /* look ahead token */
  Token lookahead2;
  struct FuncState *fs;  /* current function state */
  struct lua_State *L;
  ZIO *z;  /* input stream */
  Mbuffer *buff;  /* buffer for tokens */
  Table *h;  /* to avoid state-less tokens */
  struct Dyndata *dyd;  /* dynamic structures used by the parser */
  TString *source;  /* current source name */
  TString *envn;  /* environment variable name */
  int interp_delim;
  int interp_step;
  int interp_brace_level;
  TString *interp_temp_ts;
} LexState;


LUAI_FUNC void luaX_init (lua_State *L);
LUAI_FUNC void luaX_setinput (lua_State *L, LexState *ls, ZIO *z,
                              TString *source, int firstchar);
LUAI_FUNC TString *luaX_newstring (LexState *ls, const char *str, size_t l);
LUAI_FUNC void luaX_next (LexState *ls);
LUAI_FUNC int luaX_lookahead (LexState *ls);
LUAI_FUNC int luaX_lookahead2 (LexState *ls);
LUAI_FUNC l_noret luaX_syntaxerror (LexState *ls, const char *s);
LUAI_FUNC const char *luaX_token2str (LexState *ls, int token);


#endif
