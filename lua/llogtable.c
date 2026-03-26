#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"

static int l_index(lua_State *L) {
    lua_getfield(L, lua_upvalueindex(1), "log");
    lua_pushvalue(L, 2);
    lua_pushstring(L, " read");
    lua_concat(L, 2);
    lua_rawseti(L, -2, lua_rawlen(L, -2) + 1);
    lua_pop(L, 1);
    lua_pushvalue(L, 2);
    lua_rawget(L, lua_upvalueindex(1));
    return 1;
}

static int l_newproxy(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setfield(L, 1, "log");
    lua_newtable(L);
    lua_pushvalue(L, 1);
    lua_pushcclosure(L, l_index, 1);
    lua_setfield(L, -2, "__index");
    lua_setmetatable(L, 1);
    lua_pushvalue(L, 1);
    return 1;
}

static const luaL_Reg loglib[] = {
    {"proxy", l_newproxy},
    {NULL, NULL}
};

LUAMOD_API int luaopen_logtable(lua_State *L) {
    luaL_newlib(L, loglib);
    return 1;
}
