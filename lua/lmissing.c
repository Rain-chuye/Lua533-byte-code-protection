#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

static int stub(lua_State *L) { return 0; }

LUAMOD_API int luaopen_lclass(lua_State *L) { lua_newtable(L); return 1; }
LUAMOD_API int luaopen_lboolib(lua_State *L) { lua_newtable(L); return 1; }
LUAMOD_API int luaopen_ludatalib(lua_State *L) { lua_newtable(L); return 1; }
LUAMOD_API int luaopen_lsmgrlib(lua_State *L) { lua_newtable(L); return 1; }
LUAMOD_API int luaopen_ltranslator(lua_State *L) { lua_newtable(L); return 1; }
LUAMOD_API int luaopen_lvmlib(lua_State *L) { lua_newtable(L); return 1; }
