#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <string.h>

static int class_index(lua_State *L) {
    // 1: instance, 2: key
    lua_pushvalue(L, lua_upvalueindex(1)); // The class table
    lua_pushvalue(L, 2);
    lua_rawget(L, -2);
    if (!lua_isnil(L, -1)) return 1;
    lua_pop(L, 1);

    // Check getters
    lua_getfield(L, -1, "__getters");
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, 1);
            lua_call(L, 1, 1);
            return 1;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    // Fallback to __base
    lua_getfield(L, -1, "__base");
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_gettable(L, -2); // Base can be indexed normally
        return 1;
    }
    return 1;
}

static int class_newindex(lua_State *L) {
    // 1: instance, 2: key, 3: value
    if (!lua_getmetatable(L, 1)) return 0; // The class table

    // Check setters
    lua_getfield(L, -1, "__setters");
    if (lua_istable(L, -1)) {
        lua_pushvalue(L, 2);
        lua_rawget(L, -2);
        if (lua_isfunction(L, -1)) {
            lua_pushvalue(L, 1);
            lua_pushvalue(L, 3);
            lua_call(L, 2, 0);
            return 0;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);

    lua_pushvalue(L, 2);
    lua_pushvalue(L, 3);
    lua_rawset(L, 1);
    return 0;
}

static int Lnewclass(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    lua_newtable(L); // The class table

    if (!lua_isnoneornil(L, 2)) {
        lua_pushvalue(L, 2);
        lua_setfield(L, -2, "__base");

        // Inherit methods from base
        lua_newtable(L); // class metatable
        lua_pushvalue(L, 2);
        lua_setfield(L, -2, "__index");
        lua_setmetatable(L, -2);
    }

    lua_pushstring(L, name);
    lua_setfield(L, -2, "__name");

    lua_newtable(L);
    lua_setfield(L, -2, "__getters");
    lua_newtable(L);
    lua_setfield(L, -2, "__setters");

    // Instances will use this as metatable
    lua_pushvalue(L, -1);
    lua_pushcclosure(L, class_index, 1);
    lua_setfield(L, -2, "__index");

    lua_pushcfunction(L, class_newindex);
    lua_setfield(L, -2, "__newindex");

    return 1;
}

static int Linstantiate(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE); // The class
    lua_newtable(L); // The instance
    lua_pushvalue(L, 1);
    lua_setmetatable(L, -2);

    // Call constructor if exists
    lua_getfield(L, 1, "constructor");
    if (lua_isfunction(L, -1)) {
        int narg_total = lua_gettop(L) - 2; // args + instance
        lua_pushvalue(L, -2); // self (instance)
        for (int i = 2; i < narg_total + 1; i++) {
            lua_pushvalue(L, i);
        }
        lua_call(L, narg_total, 0);
    } else {
        lua_pop(L, 1);
    }
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
