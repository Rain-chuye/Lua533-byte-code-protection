#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

static int l_ls(lua_State *L) {
    const char *path = luaL_optstring(L, 1, ".");
    DIR *d = opendir(path);
    if (!d) return luaL_error(L, "无法打开目录: %s", strerror(errno));
    lua_newtable(L);
    struct dirent *entry;
    int i = 1;
    while ((entry = readdir(d)) != NULL) {
        lua_pushstring(L, entry->d_name);
        lua_rawseti(L, -2, i++);
    }
    closedir(d);
    return 1;
}

static int l_mkdir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    int res = mkdir(path, 0777);
    lua_pushboolean(L, res == 0);
    return 1;
}

static int l_exists(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    lua_pushboolean(L, access(path, F_OK) == 0);
    return 1;
}

static const luaL_Reg fslib[] = {
    {"ls", l_ls},
    {"mkdir", l_mkdir},
    {"exists", l_exists},
    {NULL, NULL}
};

LUAMOD_API int luaopen_fs(lua_State *L) {
    luaL_newlib(L, fslib);
    return 1;
}
