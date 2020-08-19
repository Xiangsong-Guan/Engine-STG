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
    std::swap(this->where, o.where);
}

void STGThinker::Active(int id, SCPatternsCode ptn, SCPatternData pd, const b2Body *body) noexcept
{
    pattern = patterns[static_cast<int>(ptn)];
    data = std::move(pd);
    physics = body;
    ID = id;
    where = 0;

    if (ptn == SCPatternsCode::GO_ROUND)
        data.Round.r_sq = (body->GetPosition() - data.Round.p).LengthSquared();

    /* Save AI out of data union. AI can use data to execute sub pattern. */
    AI = data.AI;
    sub_ptn = SCPatternsCode::CONTROLLED;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGThinker::Think()
{
    if (pattern(this))
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

std::function<bool(STGThinker *)> STGThinker::patterns[static_cast<int>(SCPatternsCode::NUM)];

void STGThinker::InitSCPattern()
{
    patterns[static_cast<int>(SCPatternsCode::GO_ROUND)] = std::mem_fn(&STGThinker::go_round);
    patterns[static_cast<int>(SCPatternsCode::MOVE_LAST)] = std::mem_fn(&STGThinker::move_last);
    patterns[static_cast<int>(SCPatternsCode::MOVE_TO)] = std::mem_fn(&STGThinker::move_to);
    patterns[static_cast<int>(SCPatternsCode::MOVE_PASSBY)] = std::mem_fn(&STGThinker::move_passby);
    patterns[static_cast<int>(SCPatternsCode::CONTROLLED)] = std::mem_fn(&STGThinker::controlled);
    /* Pattern "STAY" will not has thinker obj. */
}

bool STGThinker::controlled()
{
    int good;
    int event_bit = 0b0;
    int rn = 0;
    ALLEGRO_EVENT event;

    /* Pass porxy firstly. */
    lua_pushlightuserdata(AI, this);

    /* Execute sub-pattern, if exists. */
    if (sub_ptn != SCPatternsCode::CONTROLLED)
        if (patterns[static_cast<int>(sub_ptn)](this))
            /* Notify Lua this pattern end. */
            event_bit += 1;

    /* Pass STG char events */
    while (al_get_next_event(Recv, &event))
    {
        // ...
    }

    /* AI online. If AI return, means dead. */
    good = lua_resume(AI, nullptr, 1, &rn);

#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG thinker lua error: " << lua_tostring(AI, -1) << std::endl;
    else if (rn != 0)
        std::cerr << "STG thinker lua return something stupid!\n";
#endif

    if (good != LUA_YIELD)
        return true;

    return false;
}

bool STGThinker::move_to()
{
    vec4u = data.Vec - physics->GetPosition();

    if (vec4u.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
        return true;

    return false;
}

bool STGThinker::move_last()
{
    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
    event.user.data2 = reinterpret_cast<intptr_t>(&data.Vec);
    al_emit_user_event(InputMaster, &event, nullptr);

    return false;
}

bool STGThinker::move_passby()
{
    vec4u = data.Passby.Vec[where] - physics->GetPosition();

    if (vec4u.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
    {
        where += 1;
        if (where == data.Passby.Num)
            if (data.Passby.Loop)
                where = 0;
            else
                return true;
    }

    return false;
}

bool STGThinker::go_round()
{
    static constexpr float EP = .01f;
    static b2Rot ADJUST = b2Rot(.05f);

    b2Vec2 d2p = data.Round.p - physics->GetPosition();
    vec4u = data.Round.dir * d2p.Skew();

    /* rhc: skew -> ccw. Here we use lhc, so skew -> cw. And then dir > 0 -> ccw.
     * Here mul -> cw; mult -> ccw. And adjust direction should always be same with dir.
     * (Confirm EP always make circle lager, adjust always go inside.) */
    if (d2p.LengthSquared() - data.Round.r_sq > EP)
        vec4u = data.Round.dir > 0.f ? b2MulT(ADJUST, vec4u) : b2Mul(ADJUST, vec4u);

    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
    event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
    al_emit_user_event(InputMaster, &event, nullptr);

    return false;
}