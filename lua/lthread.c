#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

typedef struct {
    char *code;
    lua_Alloc f;
    void *ud;
} thread_data_t;

static void *thread_func(void *arg) {
    thread_data_t *td = (thread_data_t *)arg;
    lua_State *L = lua_newstate(td->f, td->ud);
    luaL_openlibs(L);
    if (luaL_dostring(L, td->code) != LUA_OK) {
        fprintf(stderr, "Thread error: %s\n", lua_tostring(L, -1));
    }
    lua_close(L);
    free(td->code);
    free(td);
    return NULL;
}

static int l_create(lua_State *L) {
    const char *code = luaL_checkstring(L, 1);
    thread_data_t *td = malloc(sizeof(thread_data_t));
    td->code = strdup(code);
    td->f = lua_getallocf(L, &td->ud);

    pthread_t thread;
    if (pthread_create(&thread, NULL, thread_func, td) != 0) {
        free(td->code);
        free(td);
        return 0;
    }
    lua_pushlightuserdata(L, (void *)thread);
    return 1;
}

static int l_join(lua_State *L) {
    pthread_t thread = (pthread_t)lua_touserdata(L, 1);
    pthread_join(thread, NULL);
    return 0;
}

static const luaL_Reg threadlib[] = {
    {"create", l_create},
    {"join", l_join},
    {NULL, NULL}
};

LUAMOD_API int luaopen_thread(lua_State *L) {
    luaL_newlib(L, threadlib);
    return 1;
}
