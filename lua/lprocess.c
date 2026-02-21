#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>

static int l_fork(lua_State *L) {
    pid_t pid = fork();
    lua_pushinteger(L, (lua_Integer)pid);
    return 1;
}

static int l_wait(lua_State *L) {
    int status;
    pid_t pid = wait(&status);
    lua_pushinteger(L, (lua_Integer)pid);
    lua_pushinteger(L, (lua_Integer)status);
    return 2;
}

static int l_readmem(lua_State *L) {
    pid_t pid = (pid_t)luaL_checkinteger(L, 1);
    unsigned long addr = (unsigned long)luaL_checkinteger(L, 2);
    size_t len = (size_t)luaL_checkinteger(L, 3);
    char buf[256];
    char path[64];
    sprintf(path, "/proc/%d/mem", pid);
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    lseek(fd, addr, SEEK_SET);
    char *data = malloc(len);
    ssize_t n = read(fd, data, len);
    close(fd);
    if (n > 0) {
        lua_pushlstring(L, data, n);
    } else {
        lua_pushnil(L);
    }
    free(data);
    return 1;
}

static int l_writemem(lua_State *L) {
    pid_t pid = (pid_t)luaL_checkinteger(L, 1);
    unsigned long addr = (unsigned long)luaL_checkinteger(L, 2);
    size_t len;
    const char *data = luaL_checklstring(L, 3, &len);
    char path[64];
    sprintf(path, "/proc/%d/mem", pid);
    int fd = open(path, O_WRONLY);
    if (fd < 0) return 0;
    lseek(fd, addr, SEEK_SET);
    ssize_t n = write(fd, data, len);
    close(fd);
    lua_pushinteger(L, (lua_Integer)n);
    return 1;
}

static const luaL_Reg proclib[] = {
    {"fork", l_fork},
    {"wait", l_wait},
    {"readmem", l_readmem},
    {"writemem", l_writemem},
    {NULL, NULL}
};

LUAMOD_API int luaopen_process(lua_State *L) {
    luaL_newlib(L, proclib);
    return 1;
}
