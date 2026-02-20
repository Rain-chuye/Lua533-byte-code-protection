#include <string.h>
#include <stdlib.h>
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

/* --- JSON (Placeholder/Minimal) --- */
/* For a real project, use a full library. Here we'll provide a minimal one. */
static int l_json_encode(lua_State *L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    // Dummy implementation
    lua_pushstring(L, "{\"status\": \"encoded\"}");
    return 1;
}

static int l_json_decode(lua_State *L) {
    luaL_checkstring(L, 1);
    // Dummy implementation
    lua_newtable(L);
    lua_pushstring(L, "decoded");
    lua_setfield(L, -2, "status");
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
