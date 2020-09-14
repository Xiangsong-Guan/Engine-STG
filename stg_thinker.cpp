#include "stg_thinker.h"

#include "game_event.h"
#include "cppsuckdef.h"
#include "llauxlib.h"

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
#ifdef _DEBUG
    std::cout << "Thinker-" << o.ID << " is moving, while " << ID << " is flawed.\n";
#endif

    this->ID = o.ID;
    this->physics = o.physics;
    this->data = o.data;
    this->pattern = o.pattern;
    this->where = o.where;
    this->ai = o.ai;
    this->sub_ptn = o.sub_ptn;
    this->Communication = o.Communication;
    std::swap(this->InputMaster, o.InputMaster);
    std::swap(this->Recv, o.Recv);
}

void STGThinker::Active(int id, SCPatternsCode ptn, SCPatternData pd, const b2Body *body) noexcept
{
#ifdef _DEBUG
    std::cout << "Activing thinker: ID: " << id << "; Pattern: " << SC_PATTERNS_CODE[ptn] << ";\n";
#endif

    pattern = patterns[ptn];
    data = std::move(pd);
    physics = body;
    ID = id;

    where = 0;
    /* Game over event always notified. */
    Communication = {0, 0u, 0b1u};

    if (ptn == SCPatternsCode::SCPC_GO_ROUND)
        data.round.R_SQ = (body->GetPosition() - data.round.P).LengthSquared();
    else if (ptn == SCPatternsCode::SCPC_CONTROLLED)
    {
        /* Save AI out of data union. AI can use data to execute sub pattern. */
        ai = data.ai;
        sub_ptn = SCPatternsCode::SCPC_CONTROLLED;
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGThinker::Think()
{
    if (pattern(this))
    {
#ifdef _DEBUG
        std::cout << "Thinker-" << ID << " stop thinking.\n";
#endif

        Con->DisableThr(ID);
    }
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
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    luaL_checknumber(L, 2);
    luaL_checknumber(L, 3);
#endif

    STGThinker *thinker = reinterpret_cast<STGThinker *>(lua_touserdata(L, 1));
    thinker->vec4u.Set(luaL_checknumber(L, 2), luaL_checknumber(L, 3));

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_MOVE_XY;
    event.user.data2 = reinterpret_cast<intptr_t>(&thinker->vec4u);
    al_emit_user_event(thinker->InputMaster, &event, nullptr);

    return 0;
}

/* Tell charactor to fire
 * 1=thinker
 * no return */
static int fire(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_STG_FIRE;
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID << " want to fire.\n";
#endif

    return 0;
}

/* Tell charactor to hold fire
 * 1=thinker
 * no return */
static int cease(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_STG_CEASE;
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID << " want to cease fire.\n";
#endif

    return 0;
}

/* Tell charactor to chain the shooters.
 * 1=thinker, 2=num
 * no return */
static int chain(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    luaL_checkinteger(L, 2);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_STG_CHAIN;
    event.user.data2 = lua_tointeger(L, 2);
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID
              << " want to chain " << event.user.data2 << " shooters.\n";
#endif

    return 0;
}

/* Tell charactor to disconnect the shooters chain.
 * 1=thinker
 * no return */
static int unchain(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_STG_UNCHAIN;
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID << " want to unchain shooters.\n";
#endif

    return 0;
}

/* Tell charactor to shift
 * 1=thinker
 * no return */
static int shift(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_STG_CHANGE;
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID << " want to shift.\n";
#endif

    return 0;
}

/* Tell charactor to not die yet.
 * 1=thinker, 2=pre_load_id
 * no return */
static int respwan(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    luaL_checkinteger(L, 2);
#endif

    ALLEGRO_EVENT event;
    event.user.data1 = STGCharCommand::SCC_RESPAWN;
    event.user.data2 = lua_tointeger(L, 2) - 1;
    al_emit_user_event(reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->InputMaster, &event, nullptr);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID
              << " want to respwan with resource-" << event.user.data2 << ".\n";
#endif

    return 0;
}

/* Event happenned?
 * 1=thinker, 2=event_bit_string
 * return true if happenned */
static int is_now(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    luaL_checktype(L, 2, LUA_TSTRING);
#endif

    lua_pushboolean(L, reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->Communication.EventBits &
                           STG_CHAR_EVENT_BIT[luaL_checkoption(L, 2, nullptr, STG_CHAR_EVENT)]);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID
              << " asked for event \"" << lua_tostring(L, 2) << "\". The answer is " << lua_toboolean(L, -1) << ".\n";
#endif

    return 1;
}

/* Wait by time or specific event. No para means wait to die.
 * 1=thinker, [2=time/event_str, 3=event_str...]
 * no return */
static int wait(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID << " want to wait. ";
#endif

    auto &c = reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->Communication;

    int i = 3;
    if (lua_isnumber(L, 2))
    {
        c.WaitTime = std::lroundf(lua_tonumber(L, 2));

#ifdef _DEBUG
        std::cout << "For time: " << c.WaitTime << "; ";
#endif
    }
    else
        i = 2;

    for (; i <= lua_gettop(L); i++)
    {
        c.NotifyMask += STG_CHAR_EVENT_BIT[luaL_checkoption(L, i, nullptr, STG_CHAR_EVENT)];

#ifdef _DEBUG
        std::cout << "\"" << lua_tostring(L, i) << "\"; ";
#endif
    }

#ifdef _DEBUG
    std::cout << "\n";
#endif

    return 0;
}

/* Tell charactor to use pattern.
 * 1=thinker (just use aux function)
 * no return */
static int use_pattern(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    STGThinker *thinker = reinterpret_cast<STGThinker *>(lua_touserdata(L, 1));
    thinker->sub_ptn = use_sc_pattern(L, 2, thinker->data);

#ifdef _DEBUG
    std::cout << "Thinker-" << reinterpret_cast<STGThinker *>(lua_touserdata(L, 1))->ID
              << " want to use sub-pattern: " << lua_tostring(L, 2) << "\n";
#endif

    return 0;
}

static const luaL_Reg mind_con[] = {
    {"move", move},
    {"fire", fire},
    {"cease", cease},
    {"chain", chain},
    {"unchain", unchain},
    {"shift", shift},
    {"respwan", respwan},

    {"is_now", is_now},
    {"wait", wait},
    {"use_pattern", use_pattern},
    {NULL, NULL}};

void STGThinker::HandoverController(lua_State *L)
{
    luaL_newlib(L, mind_con);
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
    int good = LUA_YIELD;
    int rn = 0;
    ALLEGRO_EVENT event;

    /* Execute sub-pattern, if exists. */
    if (sub_ptn != SCPatternsCode::SCPC_CONTROLLED)
        if (patterns[sub_ptn](this))
        {
#ifdef _DEBUG
            std::cout << "Thinker-" << ID << " sub-pattern done: " << SC_PATTERNS_CODE[sub_ptn] << "\n";
#endif

            sub_ptn = SCPatternsCode::SCPC_CONTROLLED;
            Communication.EventBits |= STGCharEvent::SCE_SUB_PATTERN_DONE;
        }

    /* Hold STG char events */
    while (al_get_next_event(Recv, &event))
    {
#ifdef _DEBUG
        std::cout << "Thinker-" << ID << " event: " << event.user.data1 << "\n";
#endif

        Communication.EventBits |= event.user.data1;
    }

#ifdef _DEBUG
    if (Communication.WaitTime == 0)
        std::cout << "Thinker-" << ID << " time wait done.\n";
#endif

    /* If something happenned, notify lua. Lua will ask what happenned when it is thinking. */
    if (Communication.EventBits & Communication.NotifyMask || Communication.WaitTime == 0)
    {
#ifdef _DEBUG
        std::cout << "Thinker-" << ID << " will resume the AI.\n";
#endif

        /* Pass porxy firstly. */
        lua_pushlightuserdata(ai, this);

        /* Death always be notified. */
        Communication.NotifyMask = 0b1u;
        Communication.WaitTime = -1;
        good = lua_resume(ai, nullptr, 1, &rn);
        /* Event just stay one loop for lua. */
        Communication.EventBits = 0u;
    }
    else if (Communication.WaitTime > 0)
        Communication.WaitTime -= 1;

#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG thinker lua error: " << lua_tostring(ai, -1) << std::endl;
#endif

    /* AI online. If AI return, means no need to further thinking. */
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