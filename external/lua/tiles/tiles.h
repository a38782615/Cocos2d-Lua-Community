#ifndef _TILES_H_
#define _TILES_H_

#include "lua.h"
#include "lauxlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

//#define luaL_newlibtable(L,l) \
	lua_createtable(L, 0, sizeof(l) / sizeof((l)[0]) - 1)

//#define luaL_newlib(L,l)  (luaL_newlibtable(L,l), luaL_setfuncs(L,l,0))
#  define luaL_newlib(L,l) (lua_newtable(L), luaL_register(L,NULL,l))

unsigned int tile_map[120][120];

typedef struct {
	int x;
	int y;
	int w;
	int h;
}LAND_RANGE;

typedef struct _P_PT_{
	unsigned char x;
	unsigned char y;
}P_PT;

typedef struct RECORD_NODE{
	P_PT ppos;
	int Fcost;
	int Hcost;
	int Gcost;
	unsigned char status;
}R_NODE;

static LAND_RANGE land_range[6];
int get_object_type(int tile);
int is_tile_walkable(int x, int y);
int get_land_id(int tile);
int get_tile(int x, int y);

LAND_RANGE getLandRange(unsigned char land);

int luaopen_tiles(lua_State *L);

#endif