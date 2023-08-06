
#ifndef __LUA_LBITLIB_H_
#define __LUA_LBITLIB_H_

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#define LUA_LIB

LUA_LIB int luaopen_bit32(lua_State* L);

#endif // __LUA_LBITLIB_H_
