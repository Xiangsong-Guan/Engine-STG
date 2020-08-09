#include "stg_thinker.h"

#include "game_event.h"
#include "cppsuckdef.h"

#include <lua.hpp>

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
    std::swap(this->physics, o.physics);
    std::swap(this->data, o.data);
    std::swap(this->pattern, o.pattern);
}

void STGThinker::Active(int id, SCPatternsCode ptn, SCPatternData pd, const b2Body *body) noexcept
{
    pattern = patterns[static_cast<int>(ptn)];
    data = std::move(pd);
    physics = body;
    ID = id;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGThinker::Think()
{
    pattern(this);
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

    if (dir & static_cast<int>(Movement::B_U))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::UP);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::B_D))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::DOWN);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::B_L))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::LEFT);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    if (dir & static_cast<int>(Movement::B_R))
    {
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::RIGHT);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Thinking Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<void(STGThinker *)> STGThinker::patterns[static_cast<int>(SCPatternsCode::NUM)];

void STGThinker::InitSCPattern()
{
    patterns[static_cast<int>(SCPatternsCode::MOVE_LAST)] = std::mem_fn(&STGThinker::move_last);
    patterns[static_cast<int>(SCPatternsCode::MOVE_TO)] = std::mem_fn(&STGThinker::move_to);
    patterns[static_cast<int>(SCPatternsCode::MOVE_PASSBY)] =
        std::mem_fn(&STGThinker::move_passby);
    patterns[static_cast<int>(SCPatternsCode::CONTROLLED)] = std::mem_fn(&STGThinker::controlled);
    /* Pattern "STAY" will not has thinker obj. */
}

void STGThinker::controlled()
{
    int good;
    int event_bit = 0b0;
    int rn = 0;
    ALLEGRO_EVENT event;

    /* Pass porxy firstly. */
    lua_pushlightuserdata(data.AI, this);

    /* Pass STG char events */
    while (al_get_next_event(Recv, &event))
    {
        // ...
    }

    /* AI online. If AI return, means dead. */
    good = lua_resume(data.AI, nullptr, 1, &rn);

#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG thinker lua error: " << lua_tostring(data.AI, -1) << std::endl;
    else if (rn != 0)
        std::cerr << "STG thinker lua return something stupid!\n";
#endif

    if (good != LUA_YIELD)
        Con->DisableThr(ID);
}

void STGThinker::move_to()
{
    b2Vec2 vec = b2Vec2(data.Vec.X, data.Vec.Y) - physics->GetPosition();

    if (vec.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = std::lroundf(vec.x * 1000.f);
        event.user.data3 = std::lroundf(vec.y * 1000.f);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
        Con->DisableThr(ID);
}

void STGThinker::move_last()
{
    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
    event.user.data2 = std::lroundf(data.Vec.X * 1000.f);
    event.user.data3 = std::lroundf(data.Vec.Y * 1000.f);
    al_emit_user_event(InputMaster, &event, nullptr);
}

void STGThinker::move_passby()
{
    b2Vec2 vec =
        b2Vec2(data.Passby.Vec[data.Passby.Where].X, data.Passby.Vec[data.Passby.Where].Y) -
        physics->GetPosition();

    if (vec.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = std::lroundf(vec.x * 1000.f);
        event.user.data3 = std::lroundf(vec.y * 1000.f);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
    {
        data.Passby.Where += 1;
        if (data.Passby.Where == data.Passby.Num)
            if (data.Passby.Loop)
                data.Passby.Where = 0;
            else
                Con->DisableThr(ID);
    }
}