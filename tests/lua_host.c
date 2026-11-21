#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 3)
        return 2;
    lua_State *L = luaL_newstate();
    luaL_openlibs(L);
    lua_pushstring(L, argv[2]);
    lua_setglobal(L, "data_root");
    int e = luaL_dofile(L, argv[1]);
    if (e)
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_close(L);
    return e != 0;
}
