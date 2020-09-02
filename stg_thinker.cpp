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
    this->ID = o.ID;
    this->physics = o.physics;
    this->data = o.data;
    this->pattern = o.pattern;
    this->where = o.where;
    this->ai = o.ai;
    this->sub_ptn = o.sub_ptn;
    std::swap(this->InputMaster, o.InputMaster);
    std::swap(this->Recv, o.Recv);
}

void STGThinker::Active(int id, SCPatternsCode ptn, SCPatternData pd, const b2Body *body) noexcept
{
    pattern = patterns[static_cast<int>(ptn)];
    data = std::move(pd);
    physics = body;
    ID = id;
    where = 0;

    if (ptn == SCPatternsCode::SCPC_GO_ROUND)
        data.round.R_SQ = (body->GetPosition() - data.round.P).LengthSquared();

    /* Save AI out of data union. AI can use data to execute sub pattern. */
    ai = data.ai;
    sub_ptn = SCPatternsCode::SCPC_CONTROLLED;
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

/* Tell charactor to move x,y
 * 1=thinker, 2=x, 3=y
 * no return */
static int move(lua_State *L)
{
#ifndef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    static_cast<STGThinker *>(lua_touserdata(L, 1))->Move(luaL_checknumber(L, 2), luaL_checknumber(L, 3));
#else
    static_cast<STGThinker *>(lua_touserdata(L, 1))->Move(lua_tointeger(L, 2), lua_tonumber(L, 3));
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

void STGThinker::Move(float x, float y)
{
    ALLEGRO_EVENT event;

    vec4u.Set(x, y);

    event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
    event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
    al_emit_user_event(InputMaster, &event, nullptr);
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Thinking Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<bool(STGThinker *)> STGThinker::patterns[SCPatternsCode::SCPC_NUM];

void STGThinker::InitSCPattern()
{
    patterns[SCPatternsCode::SCPC_GO_ROUND] = std::mem_fn(&STGThinker::go_round);
    patterns[SCPatternsCode::SCPC_MOVE_LAST] = std::mem_fn(&STGThinker::move_last);
    patterns[SCPatternsCode::SCPC_MOVE_TO] = std::mem_fn(&STGThinker::move_to);
    patterns[SCPatternsCode::SCPC_MOVE_PASSBY] = std::mem_fn(&STGThinker::move_passby);
    patterns[SCPatternsCode::SCPC_CONTROLLED] = std::mem_fn(&STGThinker::controlled);
    /* Pattern "STAY" will not has thinker obj. */
}

bool STGThinker::controlled()
{
    int good;
    int rn = 1;
    ALLEGRO_EVENT event;

    /* Pass porxy firstly. */
    lua_pushlightuserdata(ai, this);

    /* Execute sub-pattern, if exists. */
    if (sub_ptn != SCPatternsCode::SCPC_CONTROLLED)
        if (patterns[sub_ptn](this))
        {
            sub_ptn = SCPatternsCode::SCPC_CONTROLLED;
            Communication.EventBits |= STGCharEvent::SCE_SUB_PATTERN_DONE;
        }

    /* Hold STG char events */
    while (al_get_next_event(Recv, &event))
        EventBit |= event.user.data1;

    /* If something happenned, notify lua. Lua will ask what happenned when it is thinking. */
    if (EventBit > 0u)
    {
        lua_pushboolean(ai, 1);
        rn = 2;
    }

    /* AI online. If AI return, means no need to further thinking. */
    good = lua_resume(ai, nullptr, rn, &rn);
    /* Event just stay one loop for lua. */
    EventBit = 0u;

#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG thinker lua error: " << lua_tostring(ai, -1) << std::endl;
#endif

    if (good != LUA_YIELD)
        return true;

    return false;
}

bool STGThinker::move_to()
{
    vec4u = data.vec - physics->GetPosition();

    if (vec4u.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = STGCharCommand::SCC_MOVE_XY;
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
    event.user.data1 = STGCharCommand::SCC_MOVE_XY;
    event.user.data2 = reinterpret_cast<intptr_t>(&data.vec);
    al_emit_user_event(InputMaster, &event, nullptr);

    return false;
}

bool STGThinker::move_passby()
{
    vec4u = data.passby.Vec[where] - physics->GetPosition();

    if (vec4u.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = STGCharCommand::SCC_MOVE_XY;
        event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
    {
        where += 1;
        if (where == data.passby.Num)
            if (data.passby.Loop)
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

    b2Vec2 d2p = data.round.P - physics->GetPosition();
    vec4u = data.round.Dir * d2p.Skew();

    /* rhc: skew -> ccw. Here we use lhc, so skew -> cw. And then dir > 0 -> ccw.
     * Here mul -> cw; mult -> ccw. And adjust direction should always be same with dir.
     * (Confirm EP always make circle lager, adjust always go inside.) */
    if (d2p.LengthSquared() - data.round.R_SQ > EP)
        vec4u = data.round.Dir > 0.f ? b2MulT(ADJUST, vec4u) : b2Mul(ADJUST, vec4u);

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_MOVE_XY;
    event.user.data2 = reinterpret_cast<intptr_t>(&vec4u);
    al_emit_user_event(InputMaster, &event, nullptr);

    return false;
}