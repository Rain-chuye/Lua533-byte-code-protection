/*
** $Id: ldump.c,v 2.37 2015/10/08 15:53:49 roberto Exp $
** save precompiled Lua chunks
** See Copyright Notice in lua.h
*/

#define ldump_c
#define LUA_CORE

#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "lobject.h"
#include "lstate.h"
#include "lundump.h"
#include "lopcodes.h"
#include "sha256.h"


static uint32_t my_rand(uint32_t *seed) {
    *seed = *seed * 1103515245 + 12345;
    return (*seed / 65536) % 32768;
}

static void shuffle_opcodes(int *map, uint32_t seed) {
    int i;
    uint32_t rseed = seed;
    for (i = 0; i < NUM_OPCODES; i++) map[i] = i;
    for (i = NUM_OPCODES - 1; i > 0; i--) {
        int j = my_rand(&rseed) % (i + 1);
        int temp = map[i];
        map[i] = map[j];
        map[j] = temp;
    }
}


typedef struct {
    lua_State *L;
    lua_Writer writer;
    void *data;
    int strip;
    int status;
    uint32_t timestamp;
    SHA256_CTX sha256;
    int secure;
} DumpState;


/*
** All high-level dumps go through DumpVector; you can change it to
** change the endianness of the result
*/
#define DumpVector(v,n,D)	DumpBlock(v,(n)*sizeof((v)[0]),D)

#define DumpLiteral(s,D)	DumpBlock(s, sizeof(s) - sizeof(char), D)


static void DumpBlock (const void *b, size_t size, DumpState *D) {
  if (D->status == 0 && size > 0) {
    if (D->secure) {
        uint8_t *buf = (uint8_t*)malloc(size);
        memcpy(buf, b, size);
        for (size_t i = 0; i < size; i++) {
            buf[i] ^= (D->timestamp >> ((i % 4) * 8)) & 0xFF;
        }
        sha256_update(&D->sha256, buf, size);
        lua_unlock(D->L);
        D->status = (*D->writer)(D->L, buf, size, D->data);
        lua_lock(D->L);
        free(buf);
    } else {
        lua_unlock(D->L);
        D->status = (*D->writer)(D->L, b, size, D->data);
        lua_lock(D->L);
    }
  }
}


#define DumpVar(x,D)		DumpVector(&x,1,D)


static void DumpByte (int y, DumpState *D) {
  lu_byte x = (lu_byte)y;
  DumpVar(x, D);
}


static void DumpInt (int x, DumpState *D) {
  DumpVar(x, D);
}


static void DumpNumber (lua_Number x, DumpState *D) {
  DumpVar(x, D);
}


static void DumpInteger (lua_Integer x, DumpState *D) {
  DumpVar(x, D);
}

static void DumpString (const TString *s, DumpState *D) {
  if (s == NULL)
    DumpByte(0, D);
  else {
    //nirenr mod
    //size_t size = tsslen(s) + 1;  /* include trailing '\0' */
    unsigned int size = tsslen(s) + 1;  /* include trailing '\0' */
    const char *str = getstr(s);
    if (size < 0xFF)
      DumpByte(cast_int(size), D);
    else {
      DumpByte(0xFF, D);
      DumpVar(size, D);
    }
    DumpVector(str, size - 1, D);  /* no need to save '\0' */
  }
}

static void DumpCode (const Proto *f, DumpState *D) {
  DumpInt(f->sizecode, D);
  if (D->secure) {
      int map[NUM_OPCODES];
      shuffle_opcodes(map, D->timestamp);
      Instruction *code = (Instruction*)malloc(f->sizecode * sizeof(Instruction));
      for (int i = 0; i < f->sizecode; i++) {
          OpCode op = GET_OPCODE(f->code[i]);
          Instruction inst = f->code[i];
          SET_OPCODE(inst, map[op]);
          code[i] = inst;
      }
      DumpVector(code, f->sizecode, D);
      free(code);
  } else {
      DumpVector(f->code, f->sizecode, D);
  }
}

static void DumpFunction(const Proto *f, TString *psource, DumpState *D);

static void DumpConstants (const Proto *f, DumpState *D) {
  int i;
  int n = f->sizek;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    const TValue *o = &f->k[i];
    DumpByte(ttype(o), D);
    switch (ttype(o)) {
      case LUA_TNIL:
        break;
      case LUA_TBOOLEAN:
        DumpByte(bvalue(o), D);
            break;
      case LUA_TNUMFLT:
        DumpNumber(fltvalue(o), D);
            break;
      case LUA_TNUMINT:
        DumpInteger(ivalue(o), D);
            break;
      case LUA_TSHRSTR:
      case LUA_TLNGSTR:
        DumpString(tsvalue(o), D);
            break;
      default:
        lua_assert(0);
    }
  }
}


static void DumpProtos (const Proto *f, DumpState *D) {
  int i;
  int n = f->sizep;
  DumpInt(n, D);
  for (i = 0; i < n; i++)
    DumpFunction(f->p[i], f->source, D);
}


static void DumpUpvalues (const Proto *f, DumpState *D) {
  int i, n = f->sizeupvalues;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    DumpByte(f->upvalues[i].instack, D);
    DumpByte(f->upvalues[i].idx, D);
  }
}

static void DumpDebug (const Proto *f, DumpState *D) {
  int i, n;
  n = (D->strip) ? 0 : f->sizelineinfo;
  DumpInt(n, D);
  DumpVector(f->lineinfo, n, D);
  n = (D->strip) ? 0 : f->sizelocvars;
  DumpInt(n, D);
  for (i = 0; i < n; i++) {
    DumpString(f->locvars[i].varname, D);
    DumpInt(f->locvars[i].startpc, D);
    DumpInt(f->locvars[i].endpc, D);
  }
  n = (D->strip) ? 0 : f->sizeupvalues;
  DumpInt(n, D);
  for (i = 0; i < n; i++)
    DumpString(f->upvalues[i].name, D);
}


static void DumpFunction (const Proto *f, TString *psource, DumpState *D) {
  if (D->strip || f->source == psource)
    DumpString(NULL, D);  /* no debug info or same source as its parent */
  else
    DumpString(f->source, D);
  DumpInt(f->linedefined, D);
  DumpInt(f->lastlinedefined, D);
  DumpByte(f->numparams, D);
  DumpByte(f->is_vararg, D);
  DumpByte(f->maxstacksize, D);
  DumpCode(f, D);
  DumpConstants(f, D);
  DumpUpvalues(f, D);
  DumpProtos(f, D);
  DumpDebug(f, D);
}


static void DumpHeader (DumpState *D) {
  DumpLiteral(LUA_SIGNATURE, D);
  DumpByte(LUAC_VERSION, D);
  DumpByte(LUAC_FORMAT, D);
  DumpLiteral(LUAC_DATA, D);
  DumpByte(sizeof(int), D);
  //nirenr mod
  //DumpByte(sizeof(size_t), D);
  DumpByte(sizeof(unsigned int), D);
  DumpByte(sizeof(Instruction), D);
  DumpByte(sizeof(lua_Integer), D);
  DumpByte(sizeof(lua_Number), D);
  DumpInteger(LUAC_INT, D);
  DumpNumber(LUAC_NUM, D);

  /* Security Header */
  D->timestamp = (uint32_t)time(NULL);
  D->secure = 0;
  DumpInt(D->timestamp, D);
}


/*
** dump Lua function as precompiled chunk
*/
int luaU_dump(lua_State *L, const Proto *f, lua_Writer w, void *data,
              int strip) {
  DumpState D;
  uint8_t hash[32];
  D.L = L;
  D.writer = w;
  D.data = data;
  D.strip = strip;
  D.status = 0;
  D.secure = 0;
  sha256_init(&D.sha256);

  DumpHeader(&D);

  D.secure = 1; /* Start encrypting and hashing from here */
  DumpByte(f->sizeupvalues, &D);
  DumpFunction(f, NULL, &D);

  D.secure = 0;
  sha256_final(&D.sha256, hash);
  DumpBlock(hash, 32, &D);

  return D.status;
}
