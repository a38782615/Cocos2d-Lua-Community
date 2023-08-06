#include "tiles.h"
#include <time.h>

/*
*
*	tile struct, int, 32bits
*--------------------------------------------------------------------------------------------------------------------------------------------
*              gate counter      lock-status                             TileType         land or not       garbage numbers                  zorder  
*29th-31th       27th-28            26th              23th-25th          20th-22th             19              15th-18th                     1th-14th            
*----------------------------------------------------------------------------------------------------------------------------------------------
*  0000            00                 0                0  11              00  1                1                00 11                      00 1100 1100 1100
*	 |              |                 |                |___|              |___|                |                |___|                      |_______________|                              
*	 |              |                 |                  |                  |                  |                  |                                |                    
*  unused      office gate.                            land id             3bits              1 bit             4bits                            14bits       
*              gates can          0:unlocked                                                  1:land           not used                       max value 16383
*              be overlap,        1:locked                                000-111             0:wild           deprecated
*              if 00, clear tile                            
*              to be 0.
*/

static int set_object_type(int x, int y, int iType)
{
	int tile = tile_map[x][y];
	int iSet = 0;

	switch (iType)
	{
		case 1://clear land tile.
		{
				   tile = (tile & (~(15 << 18)) | (1 << 18));
				   iSet = 1;
				   break;
		}
		case 3://taken by construction.
		{
				   tile = (tile & (~(15 << 18))) | (3 << 18);
				   iSet = 3;
				   break;
		}
		case 5://taken by device.
		{
				   tile = (tile & (~(15 << 18)) | (5 << 18));
				   iSet = 5;
				   break;
		}
		case 7://taken by land-gate.(walkable but cann't be placed anything)
		{
				   tile = (tile & (~(15 << 18)) | (7 << 18));
				   iSet = 7;
				   break;
		}
		case 9://taken by office-gate.(walkable but cann't be placed anything)
		{
					tile = (tile & (~(15 << 18)) | (9 << 18));
					iSet = 9;
					break;
		}
		case 11://garbage
		{
					tile = (tile & (~(15 << 18)) | (11 << 18));
					iSet = 11;
					break;
		}
		default:
		{
				   iSet = -1;
				   break;
		}
	}

	tile_map[x][y] = tile;

	return iSet;
}

static int get_office_gates(int tile)
{
	int ret = (tile>>26) & 3;

	return ret;
}

static int change_office_gates(int x, int y, int num)
{
	int ret = 0;
	if (-1 == num || 1 == num)
	{
		int curr_num = (tile_map[x][y] >> 26) & 3;
		curr_num += num;
		//printf("change_office_gates() x %d y %d num %d\n", x, y, curr_num);
		if ( curr_num <= 0)
		{
			//tile_map[x][y] = tile_map[x][y] & (~(3 << 26));
			set_object_type(x, y, 1);
			tile_map[x][y] = tile_map[x][y] & (~(3 << 26));//clear num
			ret = 0;
		}
		else
		{
			if (get_object_type(tile_map[x][y]) == 9)//already taken.
			{
				if (curr_num > 2)
				{
					printf("change_office_gates() ERROR attemp to set gates:%d\n", curr_num);
					ret = -1;
				}
				else
				{
					//printf("change_office_gates() set x %d  y %d  occupied by gate %d\n", x, y, curr_num);
					tile_map[x][y] = (tile_map[x][y] & (~(3 << 26))) | (curr_num << 26);
				}
			}
			else
			{
				set_object_type(x, y, 9);
				tile_map[x][y] = (tile_map[x][y] & (~(3 << 26))) | (curr_num << 26);
			}
		}
	}
	else
	{
		printf("change_office_gates() ERROR attemp to set gates:%d\n", num);
		ret = -1;
	}

	return ret;
}

int get_object_type(int tile)
{
	int ret = ( tile >> 18) & 15;

	return ret;
}

static int get_garbage_num(int tile)
{
	int ret = (tile >> 14) & 15;

	return ret;
}

static int get_tile_zorder(int tile)
{
	int ret = tile & 0x3FFF;
	if (0 > ret || 0x3FFF < ret)
	{
		printf("get_tile_zorder() error, not correct range of zorder %d\n", ret);
	}

	return ret;
}

int get_land_id(int tile)
{
	int ret = (tile >> 22 ) & 7;
	if (1 > ret || 6 < ret)
	{
		printf("get_land_id() error, not correct range of id %d tile %d\n", ret, tile);
		ret = 0;
	}

	return ret;
}

static int get_land_lockstatus(int tile)
{
	int ret = (tile >> 25) & 1;
	if (1 != ret && 0 != ret)
	{
		printf("get_land_lockstatus() error, not correct status of land %d\n", ret);
	}
	//printf("get_land_lockstatus() ret %d\n", ret);
	return ret;
}

static int is_land_tile(int tile)
{
	int ret = (tile >> 18) & 1;
	if (1 != ret && 0 != ret )
	{
		printf("is_land_tile() error, not correct type of tile %d\n", ret);
	}

	return ret;
}


/*
*
*	tile struct, int, 32bits
*--------------------------------------------------------------------------------------------------------------------------------------------
*              gate counter      lock-status                             TileType         land or not       garbage numbers                  zorder
*29th-31th       27th-28            26th              23th-25th          20th-22th             19              15th-18th                     1th-14th
*----------------------------------------------------------------------------------------------------------------------------------------------
*  0000            00                 0                0  11              00  1                1                00 11                      00 1100 1100 1100
*	 |              |                 |                |___|              |___|                |                |___|                      |_______________|
*	 |              |                 |                  |                  |                  |                  |                                |
*  unused      office gate.                            land id             3bits              1 bit             4bits                            14bits
*              gates can          0:unlocked                                                  1:land                                            max value 16383
*              be overlap,        1:locked                                000-111             0:wild
*              if 00, clear tile
*              to be 0.
*/
//initialize land, set tiles available for placing/building/walking
static int init_land_info(int landID, int begX, int begY, int width, int height)
{
	printf("init_land_info() id %d, x %d, y %d, w %d, h %d\n", landID, begX, begY, width, height);
	//server records range of land-id is between 90 and 95.
	//local id is from 1 to 6, 0 is wild-land.
	int land = landID - 89;
	if (1 > land || 6 < land)
	{
		printf("init_land_info() invalid land-id: %d\n", landID);
		return -1;
	}

	if (120 <= begX + width || 120 <= begY + height || 0 > begX || 119 < begX || 0 > begY || 119 < begY)
	{
		printf("init_land_info() invalid range of coordinate: x-%d, y-%d, w-%d, h-%d\n", begX, begY, width, height);
		return -2;
	}

	int land_info =  1 << 25;//set locked
	land_info = land_info | (land << 22);//set land id
	land_info = land_info | (1 << 18);//set land-type
	land_info = land_info & (~(3 << 26));

	//printf("init_land_info() land info %d\n", land_info);

	land_range[land - 1].x = begX;
	land_range[land - 1].y = begY;
	land_range[land - 1].w = width;
	land_range[land - 1].h = height;

	int tile = 0;
	for (int x = begX; x < begX + width; ++x)
	{
		for (int y = begY; y < begY + height; ++y)
		{
			tile = tile_map[x][y];
			tile = tile | land_info;
			tile_map[x][y] = tile;
		}
	}

	printf("init_land_info() beginTile %d endTile %d\n", tile_map[begX][begY], tile_map[begX+width-1][begY+height-1]);
	return 0;
}

//some lands are irregular, set unavailable tiles to be wild. 
static int init_trim_land(int begX, int begY, int width, int height)
{
	if (120 <= begX + width || 120 <= begY + height || 0 > begX || 119 < begX || 0 > begY || 119 < begY)
	{
		printf("init_trim_land() invalid range of coordinate: x-%d, y-%d, w-%d, h-%d\n", begX, begY, width, height);
		return -2;
	}
	//printf("init_trim_land() x %d y %d w %d h %d\n", begX, begY, width, height);
	int tile = 0;
	for (int x = begX; x < begX + width; ++x)
	{
		for (int y = begY; y < begY + height; ++y)
		{
			tile = tile_map[x][y];
			tile = tile & 0x3FFF;//no land-id and land-bit is 0. keep zorder, else set 0.
			tile_map[x][y] = tile;
		}
	}
	printf("init_trim_land() begTile %d, endTile %d\n", tile_map[begX][begY], tile_map[begX+width-1][begY+height-1]);
	return 0;
}

static int unlock_land(int landID, int begX, int begY, int width, int height)
{
	//server records range of land-id is between 90 and 95.
	//local id is from 1 to 6, 0 is wild-land.
	int land = landID - 89;
	if (1 > land || 6 < land)
	{
		printf("unlock_land() invalid land-id: %d\n", landID);
		return -1;
	}

	if (120 <= begX + width || 120 <= begY + height || 0 > begX || 119 < begX || 0 > begY || 119 < begY)
	{
		printf("unlock_land() invalid range of coordinate: x-%d, y-%d, w-%d, h-%d\n", begX, begY, width, height);
		return -2;
	}

	int tile = 0;

	for (int x = begX; x < begX + width; ++x)
	{
		for (int y = begY; y < begY + height; ++y)
		{
			tile = tile_map[x][y];
			if (1 == is_land_tile(tile))
			{
				tile = tile & (~(1 << 25));//set 30th-bit to be 0.
				tile_map[x][y] = tile;
			}
		}
	}

	return 0;
}

static int init_tile_zorder(void)
{
	int width = 120;
	int height = 120;
	int zorder = 1;
	int topX = 119;
	int topY = 119;
	int tempX = 119;
	int tempY = 119;

	for (int i = 0; i < width; ++i)
	{
		tempX = topX - i;
		tempY = topY;
		while (tempX<=topX)
		{
			tile_map[tempX][tempY] = zorder;
			zorder += 1;
			tempX += 1;
			tempY -= 1;
		}
	}

	for (int i = 1; i < height; ++i)
	{
		tempX = 0;
		tempY = topY - i;
		while (tempY>=0)
		{
			tile_map[tempX][tempY] = zorder;
			zorder += 1;
			tempX += 1;
			tempY -= 1;
		}
	}
	printf("MAX zorder %d\n", zorder);
	return 0;
}

static int setZorder(lua_State *L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("error: get wrong arguments %d\n", iArgs);
		return 0;
	}
	else
	{
		int arg1 = (int)lua_tointeger(L, 1);
		int arg2 = (int)lua_tointeger(L, 2);

		arg1 = ((arg1 & 0xFFFF0000) | arg2);
		printf("arg1 %d %x\n", arg1, arg1);

		lua_pushinteger(L, arg1);
		return 1;
	}
}

static int getZorder(lua_State *L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("c-getZorder() error: get wrong arguments %d\n", iArgs);
		return 0;
	}
	else
	{
		int x = (int)lua_tointeger(L, 1);
		int y = (int)lua_tointeger(L, 2);

		if (x < 0 || x > 119 || y < 0 || y > 119)
		{
			printf("c-getZorder get invalid coordinate x %d y %d \n", x, y);
			return 0;
		}
		int tile = tile_map[x][y];
		int zorder = get_tile_zorder(tile);
		lua_pushinteger(L, zorder);
		return 1;
	}
}

static int isLandLocked(lua_State *L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("isLandLocked() get wrong parameters %d\n", iArgs);
		return 0;
	}
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("isLandLocked() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tile = tile_map[x][y];
	int LandID = get_land_id(tile);
	if (1 > LandID && 6 < LandID)
	{
		printf("isLandLocked() x %d y %d is wild-land\n", x, y);
		return 0;
	}

	int isLand = is_land_tile(tile);
	if (0 == isLand)
	{
		printf("isLandLocked() x %d y %d %d is not land-tile\n", x, y, isLand);
		return 0;
	}

	int lockStatus = get_land_lockstatus(tile);
	lua_pushinteger(L, lockStatus);
	return 1;
}

static int changeGarbageNum(lua_State *L)
{
	int iArgs = lua_gettop(L);
	if (3 != iArgs)
	{
		printf("addGarbageNum() get wrong parameters %d\n", iArgs);
		return 0;
	}
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int num = (int)luaL_checkinteger(L, 3);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("changeGarbageNum() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tile = tile_map[x][y];
	int garbageNum = get_garbage_num(tile);
	garbageNum += num;
	if (15 < garbageNum)
	{
		printf("changeGarbageNum() failed, too mush garbage in this tile x %d y %d num %d\n", x, y, garbageNum);
		return 0;
	}
	if (0 > garbageNum)
	{
		printf("changeGarbageNum() error, garbage num less than 0\n");
		garbageNum = 0;
		tile = (tile & (~(15 << 14)));//clear garbage num.
		tile_map[x][y] = tile;
		set_object_type(x, y, 1);//set tile to be 1, empty tile.
		lua_pushinteger(L, garbageNum);
		return 1;
	}

	set_object_type(x, y, 11);//set tile type 11, taken by garbages.
	tile = (tile & (~(15 << 14))) | garbageNum;

	tile_map[x][y] = tile;

	lua_pushinteger(L, garbageNum);

	return 1;
}

static int changeOfficeGates(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (3 != iArgs)
	{
		printf("changeOfficeGates() get wrong parameters %d\n", iArgs);
		return 0;
	}

	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int num = (int)luaL_checkinteger(L, 3);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("changeOfficeGates() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	num = change_office_gates(x, y, num);

	lua_pushinteger(L, num);

	return 1;
}

static int getTileInfo(lua_State *L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("getTileInfo() get wrong parameters %d\n", iArgs);
		return 0;
	}
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	//printf("getTileInfo() x %d y %d\n", x, y);
	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("getTileInfo() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tile = tile_map[x][y];

	int lockStatus = get_land_lockstatus(tile);
	int landID = get_land_id(tile);
	if (0 != landID)
	{
		landID += 89;
	}

	int tileType = get_object_type(tile);
	int garbageNum = get_garbage_num(tile);
	int zorder = get_tile_zorder(tile);
	int officeGates = get_office_gates(tile);

	lua_newtable(L);
	lua_pushstring(L, "isLocked");
	lua_pushinteger(L, lockStatus);
	lua_settable(L, -3);

	lua_pushstring(L, "landBlock");
	lua_pushinteger(L, landID);
	lua_settable(L, -3);

	lua_pushstring(L, "objectType");
	lua_pushinteger(L, tileType);
	lua_settable(L, -3);

	lua_pushstring(L, "garbages");
	lua_pushinteger(L, garbageNum);
	lua_settable(L, -3);

	lua_pushstring(L, "officeGates");
	lua_pushinteger(L, officeGates);
	lua_settable(L, -3);

	lua_pushstring(L, "zorder");
	lua_pushinteger(L, zorder);
	lua_settable(L, -3);

	return 1;
}

static int init_all_tiles(lua_State *L)
{
	memset(tile_map, 0, sizeof(tile_map));
	init_tile_zorder();
	return 0;
}

static int getTileType(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("getTileType() get wrong parameters %d\n", iArgs);
		return 0;
	}
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("getTileType() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tile = tile_map[x][y];
	int tileType = get_object_type(tile);
	lua_pushinteger(L, tileType);

	return 1;
}

static int getLandID(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (2 != iArgs)
	{
		printf("getLandID() get wrong parameters %d\n", iArgs);
		return 0;
	}
	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("getLandID() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tile = tile_map[x][y];
	int isLand = is_land_tile(tile);
	if (0 == isLand)
	{
		//printf("getLandID() x %d y %d is not land-tile\n", x, y);
		return 0;
	}

	int landID = get_land_id(tile);
	if (0 == landID)
	{
		printf("getLandID() ERROR: invalid id %d\n", landID);
		return 0;
	}
	else
	{
		landID += 89;
	}

	lua_pushinteger(L, landID);

	return 1;
}

static int setObjectType(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (3 != iArgs)
	{
		printf("setObjectType() get wrong parameters %d\n", iArgs);
		return 0;
	}

	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int type = (int)luaL_checkinteger(L, 3);

	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("setObjectType() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	if (11 < type || 0 > type)
	{
		printf("setObjectType() get invalid type %d\n", type);
		return 0;
	}

	set_object_type(x, y, type);
	int iii = get_object_type(tile_map[x][y]);
	//printf("set_object_type old type %d now %d\n", type, iii);

	lua_pushinteger(L, type);

	return 1;
}

static int initLandInfo(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (5 != iArgs)
	{
		printf("initLandInfo() get wrong parameters %d\n", iArgs);
		return 0;
	}

	int landID = (int)luaL_checkinteger(L, 1);
	int x = (int)luaL_checkinteger(L, 2);
	int y = (int)luaL_checkinteger(L, 3);
	int w = (int)luaL_checkinteger(L, 4);
	int h = (int)luaL_checkinteger(L, 5);

	init_land_info(landID, x, y, w, h);

	return 0;
}

static int initTrimLand(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (4 != iArgs)
	{
		printf("initTrimLand() get wrong parameters %d\n", iArgs);
		return 0;
	}


	int x = (int)luaL_checkinteger(L, 1);
	int y = (int)luaL_checkinteger(L, 2);
	int w = (int)luaL_checkinteger(L, 3);
	int h = (int)luaL_checkinteger(L, 4);

	init_trim_land(x, y, w, h);

	return 0;
}

static int unlockLand(lua_State* L)
{
	int iArgs = lua_gettop(L);
	if (5 != iArgs)
	{
		printf("unlockLand() get wrong parameters %d\n", iArgs);
		return 0;
	}

	int landID = (int)luaL_checkinteger(L, 1);
	int x = (int)luaL_checkinteger(L, 2);
	int y = (int)luaL_checkinteger(L, 3);
	int w = (int)luaL_checkinteger(L, 4);
	int h = (int)luaL_checkinteger(L, 5);

	unlock_land(landID, x, y, w, h);

	return 0;
}

static int getTable(lua_State* L)
{
	lua_newtable(L);

	for (int i = 0; i < 100; ++i)
	{
		lua_pushinteger(L, i+1);
		lua_newtable(L);
		lua_pushstring(L, "x");
		lua_pushinteger(L, 1);
		lua_pushstring(L, "y");
		lua_pushinteger(L, 1);
		lua_settable(L, -5);

		lua_settable(L, -3);
		lua_settable(L, -3);
	}
	

	/*lua_pushinteger(L, 2);
	lua_newtable(L);
	lua_pushstring(L, "x");
	lua_pushinteger(L, 2);
	lua_pushstring(L, "y");
	lua_pushinteger(L, 2);
	lua_settable(L, -5);
	lua_settable(L, -3);
	lua_settable(L, -3);*/

	return 1;
}


#define MAP_WIDTH  (60)
#define MAP_HEIGHT (50)

int CLOSED = 2;
int OPENED = 1;
int searchCounter = 0;

const int found = 1, nonexistent = 2;
const int walkable = 0, unwalkable = 1;// walkability array constants

static P_PT get_a_ppt(int x, int y)
{
	P_PT p;
	p.x = x;
	p.y = y;
	return p;
}

//Create needed arrays
P_PT openList[MAP_WIDTH*MAP_HEIGHT]; //1 dimensional array holding ID# of open list items
R_NODE whichList[MAP_WIDTH][MAP_HEIGHT];  //2 dimensional array used to record 
//int pathLength[numberPeople + 1];     //stores length of the found path for critter
int pathLength = 0;

//-----------------------------------------------------------------------------
// Name: FindPath
// Desc: Finds a path using A*
//-----------------------------------------------------------------------------
//int FindPath(int pathfinderID, int startingX, int startingY,
//	int targetX, int targetY, bool stepByStep = false)
static int findPath(lua_State *L)
{
	int iArgs = lua_gettop(L);
	int startX = 0;
	int startY = 0;
	int targetX = 0;
	int targetY = 0;
	int iType = -1;
	LAND_RANGE range;

	clock_t t = clock();

	if (5 == iArgs)
	{
		startX = (int)luaL_checkinteger(L, 1);
		startY = (int)luaL_checkinteger(L, 2);
		targetX = (int)luaL_checkinteger(L, 3);
		targetY = (int)luaL_checkinteger(L, 4);
		iType = (int)luaL_checkinteger(L, 5);

		printf("FindPath() search in office %d\n", iType);
	}
	else if (4 == iArgs)
	{
		startX = (int)luaL_checkinteger(L, 1);
		startY = (int)luaL_checkinteger(L, 2);
		targetX = (int)luaL_checkinteger(L, 3);
		targetY = (int)luaL_checkinteger(L, 4);
		int tile = get_tile(startX, startY);
		//printf("startX %d startY %d tile %d\n", startX, startY, tile);
		int land = get_land_id(tile);
		range = getLandRange(land - 1);
		/*startX = 40;
		startY = 59;
		targetX = 40;
		targetY = 51;*/

		//printf("FindPath() search in land sX %d sY %d tX %d tY %d, range x %d y %d w %d h %d, searchCounter %d\n",
		//	startX, startY, targetX, targetY, range.x, range.y, range.w+range.x-1, range.h+range.y-1, searchCounter);
	}
	else
	{
		printf("FindPath() get wrong parameters %d\n", iArgs);
		return 0;
	}


	int parentXval = 0, parentYval = 0,
		a = 0, b = 0, temp = 0, corner = 0, numberOfOpenListItems = 0,
		addedGCost = 0, tempGcost = 0, path = 0,
		tempx, pathX, pathY;

	//	If target square is unwalkable, return that it's a nonexistent path.
	//if (walkability[targetX][targetY] == unwalkable)
	if (!is_tile_walkable(targetX, targetY))
	{
		printf("goto noPath\n");
		return 0;
	}

	//3.Reset some variables that need to be cleared
	/*for (x = 0; x < mapWidth; x++)
	{
	for (y = 0; y < mapHeight; y++)
	whichList[x][y] = 0;
	}*/
	//memset(whichList, 0, sizeof(whichList));
	//memset(openList, 0, sizeof(openList));
	//memset(parent, 0, sizeof(parent));
	//memset(Fcost, 0, sizeof(Fcost));
	//memset(Gcost, 0, sizeof(Gcost));
	//memset(Hcost, 0, sizeof(Hcost));

	if (searchCounter > 100)
	{
		memset(whichList, 0, sizeof(whichList));
		searchCounter = 0;
	}

	//4.Add the starting location to the open list of squares to be checked.
	numberOfOpenListItems = 1;
	//openList[1] = 1;//assign it as the top (and currently only) item in the open list, which is maintained as a binary heap (explained below)
	//openX[1] = startX; openY[1] = startY;
	openList[0] = get_a_ppt(startX, startY);
	whichList[startX - range.x][startY - range.y].Gcost = 0;
	whichList[startX - range.x][startY - range.y].ppos = get_a_ppt(startX, startY);
	OPENED = searchCounter * 2 + 1;
	CLOSED = searchCounter * 2 + 2;
	//5.Do the following until a path is found or deemed nonexistent.
	searchCounter += 1;
	do
	{
		//6.If the open list is not empty, take the first cell off of the list.
		//	This is the lowest F cost cell on the open list.
		if (numberOfOpenListItems != 0)
		{
			int tmpF = 0, tmpH = 0;
			P_PT tmpOpenlist;
			for (int i = 0; i < numberOfOpenListItems; ++i)
			{
				//if (Fcost[numberOfOpenListItems - 1] > Fcost[i])
				if (whichList[openList[numberOfOpenListItems - 1].x - range.x][openList[numberOfOpenListItems - 1].y - range.y].Fcost > whichList[openList[i].x - range.x][openList[i].y - range.y].Fcost)
				{
					/*tmpF = whichList[openList[0].x][openList[0].y].Fcost;
					whichList[openList[0].x][openList[0].y].Fcost = whichList[openList[i].x][openList[i].y].Fcost;
					whichList[openList[i].x][openList[i].y].Fcost = tmpF;

					tmpH = whichList[openList[0].x][openList[0].y].Hcost;
					whichList[openList[0].x][openList[0].y].Hcost = whichList[openList[i].x][openList[i].y].Hcost;
					whichList[openList[i].x][openList[i].y].Hcost = tmpH;*/

					tmpOpenlist = openList[numberOfOpenListItems - 1];
					openList[numberOfOpenListItems - 1] = openList[i];
					openList[i] = tmpOpenlist;
				}
			}

			//7. Pop the first item off the open list.
			//parentXval = openX[openList[1]];
			//parentYval = openY[openList[1]]; //record cell coordinates of the item
			parentXval = openList[numberOfOpenListItems - 1].x;
			parentYval = openList[numberOfOpenListItems - 1].y;
			/*printf("open %d, %d %d      ", parentXval, parentYval, numberOfOpenListItems);
			for (int i = 0; i < numberOfOpenListItems; ++i)
			{
				printf("%d,%d Fcost %d ", openList[i].x, openList[i].y, whichList[openList[i].x - range.x][openList[i].y-range.y].Fcost );
			}
			printf("close %d %d\n", parentXval, parentYval);*/
			//printf("open x %d, y %d  ", parentXval, parentYval);
			whichList[parentXval - range.x][parentYval - range.y].status = CLOSED;//add the item to the closed list

			//	Open List = Binary Heap: Delete this item from the open list, which
			//  is maintained as a binary heap. For more information on binary heaps, see:
			//	http://www.policyalmanac.org/games/binaryHeaps.htm
			numberOfOpenListItems = numberOfOpenListItems - 1;//reduce number of open list items by 1	

			//	Delete the top item in binary heap and reorder the heap, with the lowest F cost item rising to the top.
			//openList[1] = openList[numberOfOpenListItems + 1];//move the last item in the heap up to slot #1

			//7.Check the adjacent squares. (Its "children" -- these path children
			//	are similar, conceptually, to the binary heap children mentioned
			//	above, but don't confuse them. They are different. Path children
			//	are portrayed in Demo 1 with grey pointers pointing toward
			//	their parents.) Add these adjacent child squares to the open list
			//	for later consideration if appropriate (see various if statements
			//	below).
			for (b = parentYval - 1; b <= parentYval + 1; b++)
			{
				for (a = parentXval - 1; a <= parentXval + 1; a++)
				{
					//	If not off the map (do this first to avoid array out-of-bounds errors)
					if (a - range.x > -1 && b - range.y > -1 && a - range.x < MAP_WIDTH && b - range.y < MAP_HEIGHT && !(parentXval == a && parentYval == b) )
					{
						//	If not already on the closed list (items on the closed list have
						//	already been considered and can now be ignored).			
						if (whichList[a - range.x][b - range.y].status != CLOSED)
						{
							//	If not a wall/obstacle square.
							//if (walkability[a][b] != unwalkable)
							if (is_tile_walkable(a, b))
							{
								//	Don't cut across corners
								corner = walkable;
								if (a == parentXval - 1)
								{
									if (b == parentYval - 1)
									{
										if (!is_tile_walkable(parentXval - 1, parentYval) || !is_tile_walkable(parentXval, parentYval - 1))
											corner = unwalkable;
									}
									else if (b == parentYval + 1)
									{
										if (!is_tile_walkable(parentXval, parentYval + 1) || !is_tile_walkable(parentXval - 1, parentYval))
											corner = unwalkable;
									}
								}
								else if (a == parentXval + 1)
								{
									if (b == parentYval - 1)
									{
										if (!is_tile_walkable(parentXval, parentYval - 1) || !is_tile_walkable(parentXval + 1, parentYval))
											corner = unwalkable;
									}
									else if (b == parentYval + 1)
									{
										if (!is_tile_walkable(parentXval + 1, parentYval) || !is_tile_walkable(parentXval, parentYval + 1))
											corner = unwalkable;
									}
								}

								if (corner == walkable)
								{
									//	If not already on the open list, add it to the open list.			
									if (whichList[a - range.x][b - range.y].status != OPENED)
									{
										//Create a new open list item in the binary heap.
										//place the new open list item (actually, its ID#) at the bottom of the heap
										//Figure out its G cost
										if (abs(a - parentXval) == 1 && abs(b - parentYval) == 1)
											addedGCost = 14;//cost of going to diagonal squares	
										else
											addedGCost = 10;//cost of going to non-diagonal squares				
										//Gcost[a-range.x][b-range.y] = Gcost[parentXval-range.x][parentYval-range.y] + addedGCost;
										whichList[a - range.x][b - range.y].Gcost = whichList[parentXval - range.x][parentYval - range.y].Gcost + addedGCost;

										openList[numberOfOpenListItems] = get_a_ppt(a, b);

										//Figure out its H and F costs and parent
										//Hcost[numberOfOpenListItems] = 10 * (abs(a - targetX) + abs(b - targetY));
										//Fcost[numberOfOpenListItems] = Gcost[a-range.x][b-range.y] + Hcost[numberOfOpenListItems];
										whichList[a - range.x][b - range.y].Hcost = 10 * (abs(a - targetX) + abs(b - targetY));
										whichList[a - range.x][b - range.y].Fcost = whichList[a - range.x][b - range.y].Gcost + whichList[a - range.x][b - range.y].Hcost;
										//parentX[a][b] = parentXval; parentY[a][b] = parentYval;
										//parent[a-range.x][b-range.y] = get_a_ppt(parentXval-range.x, parentYval-range.y);
										whichList[a - range.x][b - range.y].ppos = get_a_ppt(parentXval - range.x, parentYval - range.y);

										numberOfOpenListItems = numberOfOpenListItems + 1;//add one to the number of items in the heap

										//Move the new open list item to the proper place in the binary heap.
										//Starting at the bottom, successively compare to parent items,
										//swapping as needed until the item finds its place in the heap
										//or bubbles all the way to the top (if it has the lowest F cost).
										//While item hasn't bubbled to the top (m=1)	
										/*int tmpF = 0, tmpH = 0;
										P_PT tmpOpenlist;
										for (int i = 0; i < numberOfOpenListItems; ++i)
										{
											//if (Fcost[numberOfOpenListItems - 1] > Fcost[i])
											if (whichList[openList[numberOfOpenListItems - 1].x - range.x][openList[numberOfOpenListItems - 1].y - range.y].Fcost > whichList[openList[i].x - range.x][openList[i].y - range.y].Fcost)
											{
												/*tmpF = whichList[openList[0].x][openList[0].y].Fcost;
												whichList[openList[0].x][openList[0].y].Fcost = whichList[openList[i].x][openList[i].y].Fcost;
												whichList[openList[i].x][openList[i].y].Fcost = tmpF;

												tmpH = whichList[openList[0].x][openList[0].y].Hcost;
												whichList[openList[0].x][openList[0].y].Hcost = whichList[openList[i].x][openList[i].y].Hcost;
												whichList[openList[i].x][openList[i].y].Hcost = tmpH;* /

												tmpOpenlist = openList[numberOfOpenListItems - 1];
												openList[numberOfOpenListItems - 1] = openList[i];
												openList[i] = tmpOpenlist;
											}
										}*/
										//Change whichList to show that the new item is on the open list.
										whichList[a - range.x][b - range.y].status = OPENED;
									}
									//8.If adjacent cell is already on the open list, check to see if this 
									//	path to that cell from the starting location is a better one. 
									//	If so, change the parent of the cell and its G and F costs.	
									else //If whichList(a,b) = onOpenList
									{
										//Figure out the G cost of this possible new path
										if (abs(a - parentXval) == 1 && abs(b - parentYval) == 1)
											addedGCost = 14;//cost of going to diagonal tiles	
										else
											addedGCost = 10;//cost of going to non-diagonal tiles				
										//tempGcost = Gcost[parentXval-range.x][parentYval-range.y] + addedGCost;
										tempGcost = whichList[parentXval - range.x][parentYval - range.y].Gcost + addedGCost;

										//If this path is shorter (G cost is lower) then change
										//the parent cell, G cost and F cost. 		
										if (tempGcost < whichList[a - range.x][b - range.y].Gcost)//Gcost[a-range.x][b-range.y]) //if G cost is less,
										{
											//parentX[a][b] = parentXval; //change the square's parent
											//parentY[a][b] = parentYval;
											//parent[a-range.x][b-range.y] = get_a_ppt(parentXval-range.x, parentYval-range.y);
											//Gcost[a-range.x][b-range.y] = tempGcost;//change the G cost			
											whichList[a - range.x][b - range.y].ppos = get_a_ppt(parentXval - range.x, parentYval - range.y);
											whichList[a - range.x][b - range.y].Gcost = tempGcost;
											whichList[a - range.x][b - range.y].Fcost = whichList[a - range.x][b - range.y].Gcost + whichList[a - range.x][b - range.y].Hcost;

											//Because changing the G cost also changes the F cost, if
											//the item is on the open list we need to change the item's
											//recorded F cost and its position on the open list to make
											//sure that we maintain a properly ordered open list.
											/*for (int i = 0; i < numberOfOpenListItems; ++i)
											{
												if (openList[i].x == a && openList[i].y == b)
												{
													//Fcost[i] = Gcost[a-range.x][b-range.y] + Hcost[i];
													whichList[openList[i].x-range.x][openList[i].y-range.y].Fcost = whichList[a - range.x][b - range.y].Gcost + whichList[openList[i].x-range.x][openList[i].y-range.y].Hcost;
													P_PT tmpPointOfList;
													int tmpF = 0;
													for (int j = 0; j < numberOfOpenListItems; ++j)
													{
														//if (Fcost[1] >= Fcost[i])
														if (whichList[openList[0].x-range.x][openList[0].y-range.y].Fcost > whichList[openList[i].x-range.x][openList[i].y-range.y].Fcost)
														{
															//tmpF = whichList[openList[1].x][openList[1].y].Fcost;//Fcost[1];
															tmpPointOfList = openList[0];
															//whichList[openList[1].x][openList[1].y].Fcost = whichList[openList[i].x][openList[i].y].Fcost;//Fcost[1] = Fcost[i];
															openList[0] = openList[i];
															//whichList[openList[i].x][openList[i].y].Fcost = tmpF;//Fcost[i] = tmpF;
															openList[i] = tmpPointOfList;
														}
													}
													break;
												}
											}*/
											/*int tmpF = 0, tmpH = 0;
											P_PT tmpOpenlist;
											for (int i = 0; i < numberOfOpenListItems; ++i)
											{
												//if (Fcost[numberOfOpenListItems - 1] > Fcost[i])
												if (whichList[openList[numberOfOpenListItems - 1].x - range.x][openList[numberOfOpenListItems - 1].y - range.y].Fcost > whichList[openList[i].x - range.x][openList[i].y - range.y].Fcost)
												{
													/*tmpF = whichList[openList[0].x][openList[0].y].Fcost;
													whichList[openList[0].x][openList[0].y].Fcost = whichList[openList[i].x][openList[i].y].Fcost;
													whichList[openList[i].x][openList[i].y].Fcost = tmpF;

													tmpH = whichList[openList[0].x][openList[0].y].Hcost;
													whichList[openList[0].x][openList[0].y].Hcost = whichList[openList[i].x][openList[i].y].Hcost;
													whichList[openList[i].x][openList[i].y].Hcost = tmpH;* /

													tmpOpenlist = openList[numberOfOpenListItems - 1];
													openList[numberOfOpenListItems - 1] = openList[i];
													openList[i] = tmpOpenlist;
												}
											}*/
										}//If tempGcost < Gcost(a,b)
									}//else If whichList(a,b) = onOpenList	
								}//If not cutting a corner
							}//If not a wall/obstacle square.
						}//If not already on the closed list 
					}//If not off the map
				}//for (a = parentXval-1; a <= parentXval+1; a++){
			}//for (b = parentYval-1; b <= parentYval+1; b++){
		}//if (numberOfOpenListItems != 0)
		//9.If open list is empty then there is no path.	
		else
		{
			path = nonexistent; 
			printf("nonexistent, start(%d, %d)-target(%d, %d)\n", startX, startY, targetX, targetY);
			break;
		}

		//If target is added to open list then path has been found.
		if (whichList[targetX - range.x][targetY - range.y].status == OPENED)
		{
			path = found; break;
		}
	} while (true);//Do until path is found or deemed nonexistent

	//10.Save the path if it exists.
	if (path == found)
	{
		//printf("path == found\n");
		//a.Working backwards from the target to the starting location by checking
		//	each cell's parent, figure out the length of the path.
		pathX = targetX-range.x; pathY = targetY-range.y;
		pathLength = 1;
		P_PT pt;
		do
		{
			//Look up the parent of the current cell.	
			//tempx = parentX[pathX][pathY];
			//pathY = parentY[pathX][pathY];
			//pathX = tempx;
			pt = whichList[pathX][pathY].ppos; //parent[pathX][pathY];
			pathX = pt.x;
			pathY = pt.y;
			//printf("(x: %d,y: %d) ", pt.x + range.x, pt.y + range.y);
			//Figure out the path length
			pathLength += 1;
			/*if (100 < pathLength)
			{
				break;
			}*/
		} while (pathX + range.x != startX || pathY + range.y != startY);
		//printf("\n");
		//printf("search time %d\n", clock()-t);
		//printf("pathLength = %d\n", pathLength);

		//b.Resize the data bank to the right size in bytes
		//pathBank = (int*)malloc(pathLength * 2);

		//c. Now copy the path information over to the databank. Since we are
		//	working backwards from the target to the start location, we copy
		//	the information to the data bank in reverse order. The result is
		//	a properly ordered set of path data, from the first step to the
		//	last.
		pathX = targetX-range.x; pathY = targetY-range.y;
		lua_newtable(L);
		//lua_createtable(L, pathLength, pathLength * 2);
		do
		{
			lua_pushinteger(L, pathLength);
			lua_newtable(L);
			lua_pushstring(L, "x");
			lua_pushinteger(L, pathX + range.x);
			lua_pushstring(L, "y");
			lua_pushinteger(L, pathY + range.y);
			lua_settable(L, -5);
			lua_settable(L, -3);
			lua_settable(L, -3);

			pathLength -= 1;

			pt = whichList[pathX][pathY].ppos;//parent[pathX][pathY];
			pathX = pt.x;
			pathY = pt.y;
			//e.If we have reached the starting square, exit the loop.	
		} while (pathX +range.x != startX || pathY + range.y != startY);

		lua_pushinteger(L, pathLength);
		lua_newtable(L);
		lua_pushstring(L, "x");
		lua_pushinteger(L, startX);
		lua_pushstring(L, "y");
		lua_pushinteger(L, startY);
		lua_settable(L, -5);
		lua_settable(L, -3);
		lua_settable(L, -3);

		//printf("return time %d\n", clock() - t);

		return 1;
	}

	return 0;
	//13.If there is no path to the selected target, set the pathfinder's
	//	xPath and yPath equal to its current location and return that the
	//	path is nonexistent.
}

LAND_RANGE getLandRange(unsigned char land)
{
	LAND_RANGE range;
	range.x = land_range[land].x;
	range.y = land_range[land].y;
	range.w = land_range[land].w;
	range.h = land_range[land].h;

	return range;
}

int is_tile_walkable(int x, int y)
{
	int ret = 0;
	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("is_tile_walkable() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	int tileType = get_object_type(tile_map[x][y]);

	if (1 == tileType || 7 == tileType || 9 == tileType || 11 == tileType)
	{
		ret = 1;
	}

	return ret;
}

int get_tile(int x, int y)
{
	if (x < 0 || x > 119 || y < 0 || y > 119)
	{
		printf("get_tile() get invalid coordinate x %d y %d \n", x, y);
		return 0;
	}

	return tile_map[x][y];
}

int getClock(lua_State *L)
{
	clock_t t = clock();
	lua_pushnumber(L, t);
	return 1;
}

LUALIB_API int luaopen_tiles(lua_State *L)
{
	static const struct	luaL_Reg l[] = {
		{ "init_all_tiles", init_all_tiles },
		{ "getTileInfo", getTileInfo },
		{ "getZorder", getZorder },
		{ "getTileType", getTileType },
		{ "getLandID", getLandID },
		{ "setObjectType", setObjectType },
		{ "isLandLocked", isLandLocked },
		{ "initLandInfo", initLandInfo },
		{ "initTrimLand", initTrimLand },
		{ "unlockLand", unlockLand },
		{ "changeGarbageNum", changeGarbageNum },
		{ "changeOfficeGates", changeOfficeGates },
		{ "getTable", getTable },
		{ "findPath", findPath },
		{ "getClock", getClock },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);

	return 1;
}