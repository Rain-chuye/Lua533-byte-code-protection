#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <stdint.h>

static int l_add(lua_State *L) {
    void *p = lua_touserdata(L, 1);
    intptr_t offset = (intptr_t)luaL_checkinteger(L, 2);
    lua_pushlightuserdata(L, (void *)((char *)p + offset));
    return 1;
}

static int l_read(lua_State *L) {
    void *p = lua_touserdata(L, 1);
    lua_pushinteger(L, *(int32_t *)p);
    return 1;
}

static const luaL_Reg ptrlib[] = {
    {"add", l_add},
    {"read", l_read},
    {NULL, NULL}
};

LUAMOD_API int luaopen_ptr(lua_State *L) {
    luaL_newlib(L, ptrlib);
    return 1;
}
