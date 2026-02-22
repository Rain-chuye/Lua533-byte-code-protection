#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "lua.h"
#include "lauxlib.h"
#include "sha256.h"

/* --- SHA-256 --- */
static int l_sha256(lua_State *L) {
    size_t l;
    const char *s = luaL_checklstring(L, 1, &l);
    uint8_t hash[32];
    SHA256_CTX ctx;
    char res[65];
    sha256_init(&ctx);
    sha256_update(&ctx, (const uint8_t*)s, l);
    sha256_final(&ctx, hash);
    for (int i = 0; i < 32; i++)
        sprintf(res + (i * 2), "%02x", hash[i]);
    lua_pushstring(L, res);
    return 1;
}

static const luaL_Reg shalib[] = {
    {"hash", l_sha256},
    {NULL, NULL}
};

int luaopen_sha256(lua_State *L) {
    luaL_newlib(L, shalib);
    return 1;
}

/* --- JSON (Simplified Encoder) --- */
static void json_encode_val(lua_State *L, int idx, luaL_Buffer *b) {
    int type = lua_type(L, idx);
    switch (type) {
        case LUA_TNUMBER: luaL_addstring(b, lua_tostring(L, idx)); break;
        case LUA_TBOOLEAN: luaL_addstring(b, lua_toboolean(L, idx) ? "true" : "false"); break;
        case LUA_TSTRING: {
            luaL_addchar(b, '"');
            luaL_addstring(b, lua_tostring(L, idx));
            luaL_addchar(b, '"');
            break;
        }
        case LUA_TTABLE: {
            luaL_addchar(b, '{');
            lua_pushnil(L);
            int first = 1;
            while (lua_next(L, idx < 0 ? idx - 1 : idx) != 0) {
                if (!first) luaL_addstring(b, ", ");
                first = 0;
                json_encode_val(L, -2, b); // key
                luaL_addstring(b, ": ");
                json_encode_val(L, -1, b); // value
                lua_pop(L, 1);
            }
            luaL_addchar(b, '}');
            break;
        }
        case LUA_TNIL: luaL_addstring(b, "null"); break;
        default: luaL_addstring(b, "\"<unsupported>\""); break;
    }
}

static int l_json_encode(lua_State *L) {
    luaL_Buffer b;
    luaL_buffinit(L, &b);
    json_encode_val(L, 1, &b);
    luaL_pushresult(&b);
    return 1;
}

static const char *json_parse_val(lua_State *L, const char *s) {
    while (*s && isspace(*s)) s++;
    if (*s == '{') {
        lua_newtable(L);
        s++;
        while (*s && isspace(*s)) s++;
        if (*s == '}') return s + 1;
        while (1) {
            s = json_parse_val(L, s); // key
            while (*s && (isspace(*s) || *s == ':')) s++;
            s = json_parse_val(L, s); // value
            lua_settable(L, -3);
            while (*s && isspace(*s)) s++;
            if (*s == ',') s++;
            else if (*s == '}') return s + 1;
            else break;
        }
    } else if (*s == '"') {
        const char *start = ++s;
        while (*s && *s != '"') s++;
        lua_pushlstring(L, start, s - start);
        return s + 1;
    } else if (isdigit(*s) || *s == '-') {
        char *end;
        lua_pushnumber(L, strtod(s, &end));
        return end;
    } else if (strncmp(s, "true", 4) == 0) {
        lua_pushboolean(L, 1);
        return s + 4;
    } else if (strncmp(s, "false", 5) == 0) {
        lua_pushboolean(L, 0);
        return s + 5;
    } else if (strncmp(s, "null", 4) == 0) {
        lua_pushnil(L);
        return s + 4;
    }
    return s;
}

static int l_json_decode(lua_State *L) {
    const char *s = luaL_checkstring(L, 1);
    json_parse_val(L, s);
    return 1;
}

static const luaL_Reg jsonlib[] = {
    {"encode", l_json_encode},
    {"decode", l_json_decode},
    {NULL, NULL}
};

int luaopen_json(lua_State *L) {
    luaL_newlib(L, jsonlib);
    return 1;
}

/* --- FS --- */
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

static int l_fs_ls(lua_State *L) {
    const char *path = luaL_optstring(L, 1, ".");
    DIR *d = opendir(path);
    if (!d) return 0;
    lua_newtable(L);
    struct dirent *dir;
    int i = 1;
    while ((dir = readdir(d)) != NULL) {
        lua_pushstring(L, dir->d_name);
        lua_rawseti(L, -2, i++);
    }
    closedir(d);
    return 1;
}

static int l_fs_mkdir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, mkdir(path, 0777) == 0);
    return 1;
}

static const luaL_Reg fslib[] = {
    {"ls", l_fs_ls},
    {"mkdir", l_fs_mkdir},
    {NULL, NULL}
};

int luaopen_fs(lua_State *L) {
    luaL_newlib(L, fslib);
    return 1;
}

/* --- Process --- */
static int l_proc_getpid(lua_State *L) {
    lua_pushinteger(L, getpid());
    return 1;
}

static const luaL_Reg proclib[] = {
    {"getpid", l_proc_getpid},
    {NULL, NULL}
};

int luaopen_process(lua_State *L) {
    luaL_newlib(L, proclib);
    return 1;
}
