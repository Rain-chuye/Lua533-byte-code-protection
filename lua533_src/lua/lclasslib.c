#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <string.h>

static int class_index(lua_State *L) {
    lua_settop(L, 2);
    lua_pushvalue(L, lua_upvalueindex(1)); // 3: cls
    for (int i = 0; i < 64; i++) {
        lua_pushvalue(L, 2);
        if (lua_rawget(L, 3) != LUA_TNIL) return 1;
        lua_pop(L, 1);

        lua_pushstring(L, "__getters");
        if (lua_rawget(L, 3) == LUA_TTABLE) {
            lua_pushvalue(L, 2);
            if (lua_rawget(L, -2) == LUA_TFUNCTION) {
                lua_pushvalue(L, 1);
                lua_call(L, 1, 1);
                return 1;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        lua_pushstring(L, "__base");
        if (lua_rawget(L, 3) != LUA_TTABLE) break;
        lua_replace(L, 3);
    }
    return 0;
}

static int class_newindex(lua_State *L) {
    lua_settop(L, 3);
    lua_pushvalue(L, lua_upvalueindex(1)); // 4: cls
    for (int i = 0; i < 64; i++) {
        lua_pushstring(L, "__setters");
        if (lua_rawget(L, 4) == LUA_TTABLE) {
            lua_pushvalue(L, 2);
            if (lua_rawget(L, -2) == LUA_TFUNCTION) {
                lua_pushvalue(L, 1);
                lua_pushvalue(L, 3);
                lua_call(L, 2, 0);
                return 0;
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);

        lua_pushstring(L, "__base");
        if (lua_rawget(L, 4) != LUA_TTABLE) break;
        lua_replace(L, 4);
    }
    lua_settop(L, 3);
    lua_rawset(L, 1);
    return 0;
}

static int Lnewclass(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    lua_newtable(L); // T
    lua_pushstring(L, name);
    lua_setfield(L, -2, "__name");
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_pushvalue(L, 2);
        lua_setfield(L, -2, "__base");
        lua_newtable(L);
        lua_pushvalue(L, 2);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
    }
    lua_newtable(L);
    lua_setfield(L, -2, "__getters");
    lua_newtable(L);
    lua_setfield(L, -2, "__setters");

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, class_index, 1);
    lua_setfield(L, -2, "__index");

    lua_pushvalue(L, -1);
    lua_pushcclosure(L, class_newindex, 1);
    lua_setfield(L, -2, "__newindex");
    return 1;
}

static int Linstantiate(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int narg = lua_gettop(L) - 1;
    lua_newtable(L);
    int ins = lua_gettop(L);
    lua_pushvalue(L, 1);
    lua_setmetatable(L, ins);

    lua_pushvalue(L, 1);
    for (int i = 0; i < 64; i++) {
        lua_pushstring(L, "new");
        if (lua_rawget(L, -2) == LUA_TFUNCTION) break;
        lua_pop(L, 1);
        lua_pushstring(L, "constructor");
        if (lua_rawget(L, -2) == LUA_TFUNCTION) break;
        lua_pop(L, 1);
        lua_pushstring(L, "__base");
        if (lua_rawget(L, -2) != LUA_TTABLE) {
            lua_pop(L, 1);
            lua_pushnil(L);
            break;
        }
        lua_replace(L, -2);
    }
    if (lua_isfunction(L, -1)) {
        lua_pushvalue(L, ins);
        for(int i=0; i<narg; i++) lua_pushvalue(L, 2+i);
        lua_call(L, narg+1, 0);
    }
    lua_pushvalue(L, ins);
    return 1;
}

static const luaL_Reg classlib[] = {
    {"newclass", Lnewclass},
    {"instantiate", Linstantiate},
    {NULL, NULL}
};

LUAMOD_API int luaopen_lclass(lua_State *L) {
    luaL_newlib(L, classlib);
    return 1;
}
