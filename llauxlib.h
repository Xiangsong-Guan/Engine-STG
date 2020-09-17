#ifndef STG_LLAUX_LIB
#define STG_LLAUX_LIB

#include "data_struct.h"

#include <lua.hpp>

inline int luaNOERROR_checkoption(lua_State *L, int arg, const char *def, const char *const lst[])
{
    /* From lauxlib.c */
    const char *name = (def) ? luaL_optstring(L, arg, def) : lua_tostring(L, arg);
    int i;
    for (i = 0; lst[i]; i++)
        if (strcmp(lst[i], name) == 0)
            return i;

    return -1;
}

inline SCPatternsCode use_sc_pattern(lua_State *L, int pd_idx, SCPatternData &data)
{
    SCPatternsCode ptn;

#ifdef STG_LUA_API_ARG_CHECK
    ptn = static_cast<SCPatternsCode>(luaNOERROR_checkoption(L, pd_idx, "STAY", SC_PATTERNS_CODE));
    switch (ptn)
    {
    case SCPatternsCode::SCPC_CONTROLLED:
        luaL_checktype(L, pd_idx + 1, LUA_TTHREAD);
        data.ai = lua_tothread(L, pd_idx + 1);
        break;

    case SCPatternsCode::SCPC_MOVE_TO:
    case SCPatternsCode::SCPC_MOVE_LAST:
        data.vec.x = luaL_checknumber(L, pd_idx + 1);
        data.vec.y = luaL_checknumber(L, pd_idx + 2);
        break;

    case SCPatternsCode::SCPC_MOVE_PASSBY:
        luaL_checktype(L, pd_idx + 1, LUA_TBOOLEAN);
        data.passby.Loop = lua_toboolean(L, pd_idx + 1);
        data.passby.Num = 0;
        for (int i = pd_idx + 2; i < pd_idx + 2 + 8; i += 2)
            if (lua_isnumber(L, i))
            {
                data.passby.Vec[data.passby.Num].x = luaL_checknumber(L, i);
                data.passby.Vec[data.passby.Num].y = luaL_checknumber(L, i + 1);
                data.passby.Num += 1;
            }
            else
                break;
        break;

    case SCPatternsCode::SCPC_GO_ROUND:
        data.round.P.x = luaL_checknumber(L, pd_idx + 1);
        data.round.P.y = luaL_checknumber(L, pd_idx + 2);
        data.round.Dir = luaL_checknumber(L, pd_idx + 3);
        break;
    }
#else
    ptn = static_cast<SCPatternsCode>(luaNOERROR_checkoption(L, pd_idx, "STAY", SC_PATTERNS_CODE));
    switch (ptn)
    {
    case SCPatternsCode::SCPC_CONTROLLED:
        data.ai = lua_tothread(L, pd_idx + 1);
        break;

    case SCPatternsCode::SCPC_MOVE_TO:
    case SCPatternsCode::SCPC_MOVE_LAST:
        data.vec.x = lua_tonumber(L, pd_idx + 1);
        data.vec.y = lua_tonumber(L, pd_idx + 2);
        break;

    case SCPatternsCode::SCPC_MOVE_PASSBY:
        data.passby.Loop = lua_toboolean(L, pd_idx + 1);
        data.passby.Num = 0;
        for (int i = pd_idx + 2; i < pd_idx + 2 + 8; i += 2)
            if (lua_isnumber(L, i) && lua_isnumber(L, i + 1))
            {
                data.passby.Vec[data.passby.Num].x = lua_tonumber(L, i);
                data.passby.Vec[data.passby.Num].y = lua_tonumber(L, i + 1);
                data.passby.Num += 1;
            }
            else
                break;
        break;

    case SCPatternsCode::SCPC_GO_ROUND:
        data.round.P.x = lua_tonumber(L, pd_idx + 1);
        data.round.P.y = lua_tonumber(L, pd_idx + 2);
        data.round.Dir = lua_tonumber(L, pd_idx + 3);
        break;
    }
#endif

    switch (ptn)
    {
    case SCPatternsCode::SCPC_MOVE_TO:
        data.vec.x *= PHYSICAL_WIDTH;
        data.vec.y *= PHYSICAL_HEIGHT;
        break;

    case SCPatternsCode::SCPC_MOVE_PASSBY:
        for (int i = 0; i < data.passby.Num; i++)
        {
            data.passby.Vec[i].x *= PHYSICAL_WIDTH;
            data.passby.Vec[i].y *= PHYSICAL_HEIGHT;
        }
        break;

    case SCPatternsCode::SCPC_GO_ROUND:
        data.round.P.x *= PHYSICAL_WIDTH;
        data.round.P.y *= PHYSICAL_HEIGHT;
        break;
    }

    return ptn;
}

#endif