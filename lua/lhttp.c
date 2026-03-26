#define LUA_LIB
#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static int l_get(lua_State *L) {
    const char *host = luaL_checkstring(L, 1);
    const char *path = luaL_optstring(L, 2, "/");
    struct hostent *server;
    struct sockaddr_in serv_addr;
    int sockfd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return 0;

    server = gethostbyname(host);
    if (server == NULL) { close(sockfd); return 0; }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(80);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return 0;
    }

    char request[1024];
    sprintf(request, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", path, host);
    write(sockfd, request, strlen(request));

    luaL_Buffer b;
    luaL_buffinit(L, &b);
    char buffer[4096];
    int n;
    while ((n = read(sockfd, buffer, sizeof(buffer))) > 0) {
        luaL_addlstring(&b, buffer, n);
    }
    close(sockfd);
    luaL_pushresult(&b);
    return 1;
}

static const luaL_Reg httplib[] = {
    {"get", l_get},
    {NULL, NULL}
};

LUAMOD_API int luaopen_http(lua_State *L) {
    luaL_newlib(L, httplib);
    return 1;
}
