#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "lobject.h"
#include "lfunc.h"
#include "lobfuscator.h"

static int l_virtualize(lua_State *L) {
    luaL_checktype(L, 1, LUA_TFUNCTION);
    if (lua_iscfunction(L, 1))
        return luaL_error(L, "只能虚拟化 Lua 函数");

    LClosure *cl = (LClosure *)lua_topointer(L, 1);
    if (!cl) return 0;

    obfuscate_proto(L, cl->p, 1);
    lua_pushvalue(L, 1);
    return 1;
}

static const luaL_Reg vmlib[] = {
    {"virtualize", l_virtualize},
    {NULL, NULL}
};

LUAMOD_API int luaopen_vmprotect(lua_State *L) {
    luaL_newlib(L, vmlib);
    return 1;
}
