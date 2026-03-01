#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

/* Simple JSON Encoder */
static void json_encode_value(lua_State *L, int idx, luaL_Buffer *b) {
    int t = lua_type(L, idx);
    switch (t) {
        case LUA_TBOOLEAN:
            luaL_addstring(b, lua_toboolean(L, idx) ? "true" : "false");
            break;
        case LUA_TNIL:
            luaL_addstring(b, "null");
            break;
        case LUA_TNUMBER: {
            char buf[64];
            sprintf(buf, LUA_NUMBER_FMT, lua_tonumber(L, idx));
            luaL_addstring(b, buf);
            break;
        }
        case LUA_TSTRING: {
            size_t len;
            const char *s = lua_tolstring(L, idx, &len);
            luaL_addchar(b, '"');
            for (size_t i = 0; i < len; i++) {
                if (s[i] == '"' || s[i] == '\\') luaL_addchar(b, '\\');
                luaL_addchar(b, s[i]);
            }
            luaL_addchar(b, '"');
            break;
        }
        case LUA_TTABLE: {
            int is_array = 0;
            lua_pushnil(L);
            if (lua_next(L, idx < 0 ? idx - 1 : idx)) {
                if (lua_type(L, -2) == LUA_TNUMBER) is_array = 1;
                lua_pop(L, 2);
            } else is_array = 1; // Empty table as array

            if (is_array) {
                luaL_addchar(b, '[');
                int n = lua_rawlen(L, idx);
                for (int i = 1; i <= n; i++) {
                    lua_rawgeti(L, idx, i);
                    json_encode_value(L, -1, b);
                    lua_pop(L, 1);
                    if (i < n) luaL_addchar(b, ',');
                }
                luaL_addchar(b, ']');
            } else {
                luaL_addchar(b, '{');
                lua_pushnil(L);
                int first = 1;
                while (lua_next(L, idx < 0 ? idx - 1 : idx)) {
                    if (!first) luaL_addchar(b, ',');
                    first = 0;
                    lua_pushvalue(L, -2);
                    json_encode_value(L, -1, b);
                    lua_pop(L, 1);
                    luaL_addchar(b, ':');
                    json_encode_value(L, -1, b);
                    lua_pop(L, 1);
                }
                luaL_addchar(b, '}');
            }
            break;
        }
        default:
            luaL_addstring(b, "null");
            break;
    }
}

static int l_encode(lua_State *L) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    json_encode_value(L, 1, &b);
    luaL_pushresult(&b);
    return 1;
}

/* Simple JSON Decoder */
typedef struct {
    const char *s;
    size_t len;
    size_t pos;
} json_parse_t;

static void skip_ws(json_parse_t *p) {
    while (p->pos < p->len && isspace(p->s[p->pos])) p->pos++;
}

static int parse_value(lua_State *L, json_parse_t *p);

static int parse_object(lua_State *L, json_parse_t *p) {
    lua_newtable(L);
    p->pos++; // skip '{'
    while (1) {
        skip_ws(p);
        if (p->s[p->pos] == '}') { p->pos++; return 1; }
        if (!parse_value(L, p)) return 0; // key
        skip_ws(p);
        if (p->s[p->pos] != ':') return 0;
        p->pos++;
        if (!parse_value(L, p)) return 0; // value
        lua_settable(L, -3);
        skip_ws(p);
        if (p->s[p->pos] == ',') p->pos++;
        else if (p->s[p->pos] == '}') { p->pos++; return 1; }
        else return 0;
    }
}

static int parse_array(lua_State *L, json_parse_t *p) {
    lua_newtable(L);
    p->pos++; // skip '['
    int i = 1;
    while (1) {
        skip_ws(p);
        if (p->s[p->pos] == ']') { p->pos++; return 1; }
        if (!parse_value(L, p)) return 0;
        lua_rawseti(L, -2, i++);
        skip_ws(p);
        if (p->s[p->pos] == ',') p->pos++;
        else if (p->s[p->pos] == ']') { p->pos++; return 1; }
        else return 0;
    }
}

static int parse_string(lua_State *L, json_parse_t *p) {
    p->pos++; // skip '"'
    size_t start = p->pos;
    while (p->pos < p->len && p->s[p->pos] != '"') {
        if (p->s[p->pos] == '\\') p->pos++;
        p->pos++;
    }
    lua_pushlstring(L, p->s + start, p->pos - start);
    p->pos++; // skip '"'
    return 1;
}

static int parse_value(lua_State *L, json_parse_t *p) {
    skip_ws(p);
    char c = p->s[p->pos];
    if (c == '{') return parse_object(L, p);
    if (c == '[') return parse_array(L, p);
    if (c == '"') return parse_string(L, p);
    if (isdigit(c) || c == '-') {
        char *end;
        double d = strtod(p->s + p->pos, &end);
        lua_pushnumber(L, d);
        p->pos = end - p->s;
        return 1;
    }
    if (strncmp(p->s + p->pos, "true", 4) == 0) { lua_pushboolean(L, 1); p->pos += 4; return 1; }
    if (strncmp(p->s + p->pos, "false", 5) == 0) { lua_pushboolean(L, 0); p->pos += 5; return 1; }
    if (strncmp(p->s + p->pos, "null", 4) == 0) { lua_pushnil(L); p->pos += 4; return 1; }
    return 0;
}

static int l_decode(lua_State *L) {
    size_t len;
    const char *s = luaL_checklstring(L, 1, &len);
    json_parse_t p = {s, len, 0};
    if (!parse_value(L, &p)) return luaL_error(L, "JSON 解析错误");
    return 1;
}

static const luaL_Reg jsonlib[] = {
    {"encode", l_encode},
    {"decode", l_decode},
    {NULL, NULL}
};

LUAMOD_API int luaopen_json(lua_State *L) {
    luaL_newlib(L, jsonlib);
    return 1;
}
