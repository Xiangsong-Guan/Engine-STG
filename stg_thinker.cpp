#include "stg_thinker.h"

#include "game_event.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                          STGThinker Initialize / Destroy Function                             *
 *                                                                                               *
 *************************************************************************************************/

STGThinker::STGThinker()
{
    InputMaster = new ALLEGRO_EVENT_SOURCE;
    al_init_user_event_source(InputMaster);

    Recv = al_create_event_queue();

    if (!Recv)
    {
        std::cerr << "Failed to initialize STG Thinker' event queue!\n";
        std::abort();
    }
}

STGThinker::~STGThinker()
{
    al_destroy_event_queue(Recv);

    al_destroy_user_event_source(InputMaster);
    delete InputMaster;
}

void STGThinker::CPPSuckSwap(STGThinker &o) noexcept
{
    std::swap(this->ID, o.ID);
    std::swap(this->InputMaster, o.InputMaster);
    std::swap(this->Recv, o.Recv);
    std::swap(this->ai, o.ai);
}

void STGThinker::Active(int id, lua_State *co) noexcept
{
    ID = id;
    ai = co;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGThinker::Think()
{
    int good;
    int event_bit = 0b0;
    int rn = 0;
    ALLEGRO_EVENT event;

    /* Pass porxy firstly. */
    lua_pushlightuserdata(ai, this);

    /* Pass STG char events */
    while (al_get_next_event(Recv, &event))
    {
        // ...
    }

    /* AI online. If AI return, means dead. */
    good = lua_resume(ai, nullptr, 1, &rn);

#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG thinker lua error: " << lua_tostring(ai, -1) << std::endl;
    if (rn != 0)
        std::cerr << "STG thinker lua return something stupid!\n";
#endif

    if (good == LUA_OK)
        Con->DisableThr(ID);
}

/*************************************************************************************************
 *                                                                                               *
 *                                     Lua AI Commands API                                       *
 *                                                                                               *
 *************************************************************************************************/

/* Tell charactor to move in 8 direction (bit format).
 * 1=thinker, 2=8-dir-opt 
 * no return */
static int move(lua_State *L)
{
#ifndef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    static_cast<STGThinker *>(lua_touserdata(L, 1))->Move(luaL_checkinteger(L, 2));
#else
    static_cast<STGThinker *>(lua_touserdata(L, 1))->Move(lua_tointeger(L, 2));
#endif

    return 0;
}

static const luaL_Reg mind_con[] = {
    {"move", move},
    {NULL, NULL}};

void STGThinker::HandoverController(lua_State *L)
{
    luaL_newlib(L, mind_con);
}

/*************************************************************************************************
 *                                                                                               *
 *                                       AI Commands                                             *
 *                                                                                               *
 *************************************************************************************************/

void STGThinker::Move(int dir)
{
    ALLEGRO_EVENT event;

    if (dir & static_cast<int>(Movement::UP))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::UP);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::DOWN))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::DOWN);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::LEFT))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::LEFT);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::RIGHT))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::RIGHT);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
}
