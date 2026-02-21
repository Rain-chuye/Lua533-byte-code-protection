#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <string.h>
#include <stdint.h>

static int l_pack(lua_State *L) {
    const char *fmt = luaL_checkstring(L, 1);
    int n = lua_gettop(L);
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    int arg = 2;
    for (const char *p = fmt; *p; p++) {
        switch (*p) {
            case 'i': {
                int32_t v = (int32_t)luaL_checkinteger(L, arg++);
                luaL_addlstring(&b, (char *)&v, 4);
                break;
            }
            case 'f': {
                float v = (float)luaL_checknumber(L, arg++);
                luaL_addlstring(&b, (char *)&v, 4);
                break;
            }
            case 'd': {
                double v = (double)luaL_checknumber(L, arg++);
                luaL_addlstring(&b, (char *)&v, 8);
                break;
            }
            case 's': {
                size_t len;
                const char *v = luaL_checklstring(L, arg++, &len);
                luaL_addlstring(&b, v, len);
                break;
            }
        }
    }
    luaL_pushresult(&b);
    return 1;
}

static int l_unpack(lua_State *L) {
    const char *fmt = luaL_checkstring(L, 1);
    size_t len;
    const char *data = luaL_checklstring(L, 2, &len);
    size_t pos = 0;
    int count = 0;
    for (const char *p = fmt; *p && pos < len; p++) {
        switch (*p) {
            case 'i': {
                int32_t v;
                memcpy(&v, data + pos, 4);
                lua_pushinteger(L, v);
                pos += 4;
                break;
            }
            case 'f': {
                float v;
                memcpy(&v, data + pos, 4);
                lua_pushnumber(L, v);
                pos += 4;
                break;
            }
            case 'd': {
                double v;
                memcpy(&v, data + pos, 8);
                lua_pushnumber(L, v);
                pos += 8;
                break;
            }
        }
        count++;
    }
    return count;
}

static const luaL_Reg structlib[] = {
    {"pack", l_pack},
    {"unpack", l_unpack},
    {NULL, NULL}
};

LUAMOD_API int luaopen_struct(lua_State *L) {
    luaL_newlib(L, structlib);
    return 1;
}
