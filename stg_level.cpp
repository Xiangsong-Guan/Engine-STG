#include "stg_level.h"

#include "resource_manger.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                                     Lua Commands API                                          *
 *                                                                                               *
 *************************************************************************************************/

/* Aux Functions */

static inline void load_pattern(lua_State *L, int pd_idx, SCPatternsCode *ptn, SCPatternData *data)
{
#ifdef STG_LUA_API_ARG_CHECK
    *ptn = static_cast<SCPatternsCode>(luaL_checkinteger(L, pd_idx));
    switch (*ptn)
    {
    case SCPatternsCode::CONTROLLED:
        luaL_checktype(L, pd_idx + 1, LUA_TTHREAD);
        data->AI = lua_tothread(L, pd_idx + 1);
        break;

    case SCPatternsCode::MOVE_TO:
    case SCPatternsCode::MOVE_LAST:
        data->Vec.X = luaL_checknumber(L, pd_idx + 1);
        data->Vec.Y = luaL_checknumber(L, pd_idx + 2);
        break;

    case SCPatternsCode::MOVE_PASSBY:
        data->Passby.Where = 0;
        luaL_checktype(L, pd_idx + 1, LUA_TBOOLEAN);
        data->Passby.Loop = lua_toboolean(L, pd_idx + 1);
        data->Passby.Num = 0;
        for (int i = pd_idx + 2; i < pd_idx + 2 + 8; i += 2)
            if (lua_isnumber(L, i))
            {
                data->Passby.Vec[data->Passby.Num].X = luaL_checknumber(L, i);
                data->Passby.Vec[data->Passby.Num].Y = luaL_checknumber(L, i + 1);
                data->Passby.Num += 1;
            }
            else
                break;
        break;
    }
#else
    *ptn = static_cast<SCPatternsCode>(lua_tointeger(L, pd_idx));
    switch (*ptn)
    {
    case SCPatternsCode::CONTROLLED:
        data->AI = lua_tothread(L, pd_idx + 1);
        break;

    case SCPatternsCode::MOVE_TO:
    case SCPatternsCode::MOVE_LAST:
        data->Vec.X = lua_tonumber(L, pd_idx + 1);
        data->Vec.Y = lua_tonumber(L, pd_idx + 2);
        break;

    case SCPatternsCode::MOVE_PASSBY:
        data->Passby.Where = 0;
        data->Passby.Loop = lua_toboolean(L, pd_idx + 1);
        data->Passby.Num = 0;
        for (int i = pd_idx + 2; i < pd_idx + 2 + 8; i += 2)
            if (lua_isnumber(L, i) && lua_isnumber(L, i + 1))
            {
                data->Passby.Vec[data->Passby.Num].X = lua_tonumber(L, i);
                data->Passby.Vec[data->Passby.Num].Y = lua_tonumber(L, i + 1);
                data->Passby.Num += 1;
            }
            else
                break;
        break;
    }
#endif
}

/* Tell game to further load a charactor. It must debut at sometime. 
 * 1=level, 2=char_id, 3=pattern, 4...=pd
 * no return */
static int further_load(lua_State *L)
{
    int id;
    SCPatternsCode ptn;
    SCPatternData pd;
    STGLevel *level;

#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    id = luaL_checkinteger(L, 2) - 1;
#else
    id = lua_tointeger(L, 2) - 1;
#endif
    level = static_cast<STGLevel *>(lua_touserdata(L, 1));

    load_pattern(L, 3, &ptn, &pd);
    level->FillMind(id, ptn, std::move(pd));

    return 0;
}

/* Tell game to let a standby char on stage.
 * 1=level, 2=char_id, 3=pox_x, 4-pos_y
 * no return */
static int debut(lua_State *L)
{
    int id;
    STGLevel *level;
    float x, y;

#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    id = luaL_checkinteger(L, 2) - 1;
    x = luaL_checknumber(L, 3);
    y = luaL_checknumber(L, 4);
#else
    id = lua_tointeger(L, 2) - 1;
    x = lua_tonumber(L, 3);
    y = lua_tonumber(L, 4);
#endif
    level = static_cast<STGLevel *>(lua_touserdata(L, 1));

    level->Debut(id, x, y);

    return 0;
}

/* Tell game to let a new char on stage.
 * 1=level, 2=char_id, 3=pox_x, 4-pos_y, 5=pattern/thread, 6...=pd
 * no return */
static int airborne(lua_State *L)
{
    int id;
    float x, y;
    SCPatternsCode ptn;
    SCPatternData pd;
    STGLevel *level;

#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    id = luaL_checkinteger(L, 2) - 1;
    x = luaL_checknumber(L, 3);
    y = luaL_checknumber(L, 4);
#else
    id = lua_tointeger(L, 2) - 1;
    x = lua_tonumber(L, 3);
    y = lua_tonumber(L, 4);
#endif
    level = static_cast<STGLevel *>(lua_touserdata(L, 1));

    load_pattern(L, 5, &ptn, &pd);
    level->Airborne(id, x, y, ptn, std::move(pd));

    return 0;
}

static const luaL_Reg level_con[] = {
    {"further_load", further_load},
    {"debut", debut},
    {"airborne", airborne},
    {NULL, NULL}};

/*************************************************************************************************
 *                                                                                               *
 *                          STGLevel Initialize / Destroy Function                               *
 *                                                                                               *
 *************************************************************************************************/

void STGLevel::Load(int width, int height, float time_step, const STGLevelSetting &setting)
{
    /* Body define can be share, all charactors is kinematic. */
    bd.type = b2_kinematicBody;
    bd.linearDamping = 0.f;
    bd.angularDamping = 0.f;
    bd.gravityScale = 0.f;
    bd.fixedRotation = true;

    /* set right para */
    float scale = static_cast<float>(width) / static_cast<float>(height) >
                          SCREEN_WIDTH / SCREEN_HEIGHT
                      ? static_cast<float>(height) / static_cast<float>(SCREEN_HEIGHT)
                      : static_cast<float>(width) / static_cast<float>(SCREEN_WIDTH);
    this->time_step = time_step;
    bound[0] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[1] = PHYSICAL_HEIGHT + STG_FIELD_BOUND_BUFFER;
    bound[2] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[3] = PHYSICAL_WIDTH + STG_FIELD_BOUND_BUFFER;
    world = new b2World(b2Vec2(0.f, 0.f));

#ifdef STG_DEBUG_PHY_DRAW
    p_draw.Init(PIXIL_PRE_M * scale);
    world->SetDebugDraw(&p_draw);
    world->SetContactListener(&d_contact_listener);
#endif

    /* Set right thing for comps */
    for (int i = 0; i < MAX_ON_STAGE; i++)
    {
        sprite_renderers[i].SetScale(scale, scale, PIXIL_PRE_M * scale);

        onstage_thinkers[i].Con = this;
        onstage_charactors[i].Con = this;
    }

    /* pool reset */
    reset_id();
    all_state.Reset();

    /* Preload function just load all the resource, like sprite, sound, anime, etc.
     * Some in game feature, like physics, script coroutine, etc. should be initialize
     * in different time: someone in level load phase, someone in stage runtime.
     * Let stage thread first call to initialize whatever, not here. */
    for (size_t i = 0;
         i < (setting.Charactors.size() > MAX_ENTITIES ? MAX_ENTITIES : setting.Charactors.size());
         i++)
    {
        if (!our_charactor.contains(setting.Charactors[i].Char))
        {
            STGCharactorSetting sc = ResourceManager::GetSTGChar(setting.Charactors[i].Char);
            /* Tune the filter, here we can tishu. */
            sc.Phy.FD.filter.groupIndex = static_cast<int16>(CollisionType::G_OTHERS_SIDE);
            sc.Phy.FD.filter.categoryBits = static_cast<uint16>(CollisionType::C_ENEMY);
            sc.Phy.FD.filter.maskBits = static_cast<uint16>(CollisionType::M_ALL_PLAYER);

            my_charactor.push_back(std::move(sc));
            our_charactor.insert({setting.Charactors[i].Char, my_charactor.size() - 1});
            standby[i].MyChar = my_charactor.size() - 1;
        }
        else
            standby[i].MyChar = our_charactor[setting.Charactors[i].Char];

        standby[i].MyEnter = all_state.MakeChar(my_charactor[standby[i].MyChar]);
    }

    /* Get stage thread in position. (coroutine, function already on the top) */
    L_stage = ResourceManager::GetCoroutine(ResourceManager::STG_STAGE_FUNCTIONS_KEY, setting.Name);
    /* Save thread in Lua, or it will be collected. */
    lua_pushthread(L_stage);
    stage_thread_ref = luaL_ref(L_stage, LUA_REGISTRYINDEX);
    /* Initialize call. thinker lib, Level lib and level light userdata passed in. 
     * Player watching will be return. */
    int rn = 1;
    lua_State *player_watching;
    STGThinker::HandoverController(L_stage);
    luaL_newlib(L_stage, level_con);
    lua_pushlightuserdata(L_stage, this);
    /* So here the stage thread first call will tell game to further init some charactors
     * who can be compelet initialized at this phase (mainly thinker). */
#ifdef STG_LUA_API_ARG_CHECK
    int good = lua_resume(L_stage, nullptr, 3, &rn);
    if (good != LUA_YIELD)
        std::cerr << "STG stage Lua preload error: "
                  << (good == LUA_OK ? "it just return." : lua_tostring(L_stage, -1))
                  << std::endl;
    if (rn != 1 || !lua_isthread(L_stage, -1))
        std::cerr << "STG stage Lua did not return player watching!\n";
#else
    lua_resume(L_stage, nullptr, 3, &rn);
#endif
    player_watching = lua_tothread(L_stage, -1);
    lua_pop(L_stage, rn);

    /* Connect player's comps. */
    al_register_event_source(onstage_charactors[0].InputTerminal, onstage_thinkers[0].InputMaster);
    al_register_event_source(sprite_renderers[0].Recv, onstage_charactors[0].RendererMaster);
    al_register_event_source(onstage_thinkers[0].Recv, onstage_charactors[0].KneeJump);
    /* Make body and tune fixture for player */
    bd.position.Set(PHYSICAL_WIDTH * .5f, PHYSICAL_HEIGHT * .75f);
    player = world->CreateBody(&bd);
    GPlayer.Phy.FD.filter.groupIndex = static_cast<int16>(CollisionType::G_PLAYER_SIDE);
    GPlayer.Phy.FD.filter.categoryBits = static_cast<uint16>(CollisionType::C_PLAYER);
    GPlayer.Phy.FD.filter.maskBits = static_cast<uint16>(CollisionType::M_ALL_ENEMY);
    /* MUST BE 0 */
    int p_id = get_id();
    /* Player on stage. */
    SCPatternData pd;
    pd.AI = player_watching;
    onstage_charactors[0].Enable(p_id, player, GPlayer, all_state.MakeChar(GPlayer));
    sprite_renderers[0].Show(p_id, player, GPlayer.Texs.VeryFirstTex);
    onstage_thinkers[0].Active(p_id, SCPatternsCode::CONTROLLED, std::move(pd), player);
    /* id record */
    records[p_id][static_cast<int>(STGCompType::CHARACTOR)] = 0;
    records[p_id][static_cast<int>(STGCompType::RENDER)] = 0;
    records[p_id][static_cast<int>(STGCompType::THINKER)] = 0;
    /* Update information for update loop. */
    charactors_n = 1;
    thinkers_n = 1;
    sprite_renderers_n = 1;
}

/* ONLY call by outside. */
void STGLevel::Unload()
{
    /* Just stage thread is kept by game, all charactors' coroutines are kept in 
     * stage thread by Lua. So here we unref the stage thread, all things in Lua
     * will be collected. */
    lua_resetthread(L_stage);
    luaL_unref(L_stage, LUA_REGISTRYINDEX, stage_thread_ref);

    delete world;

    our_charactor.clear();
    our_shooter.clear();
    our_bullet.clear();
    my_charactor.clear();
    my_shooter.clear();
    my_bullet.clear();
}

ALLEGRO_EVENT_QUEUE *STGLevel::InputConnectionTerminal() const noexcept
{
    /* Player directly connects to game's input processor. */
    /* Gamer always at first of stage. */
    return onstage_charactors[records[0][static_cast<int>(STGCompType::CHARACTOR)]].InputTerminal;
}

/*************************************************************************************************
 *                                                                                               *
 *                          STGLevel    Update    Function                                       *
 *                                                                                               *
 *************************************************************************************************/

void STGLevel::Update()
{
    /* Thinking... Disable can be cancel here. */
    /* Disable process invoked by thinker (I think I am dead...). Thinker will notify charactor,
     * charactor will notify render... All these comps will tell level to disable themselves.
     * Simple pattern do not recieve feedback from charactor, that means they do no thinking or 
     * do reaction. Just do it. */
    for (int i = 0; i < thinkers_n; i++)
        onstage_thinkers[i].Think();

    /* Disabel happnned here. Disable state will be set when dead, but not action instantly.
     * Because disable state by "collision", "not in screen" and "stage". The state action will be
     * executed in next loop. So before that, thinker can save. */
    /* Charactors logic, command execute, state update, anime fresh.
     * Stgcharactor just store state and have a hit function for colloision to call. */
    for (int i = 0; i < charactors_n; i++)
    {
        b2Vec2 p = onstage_charactors[i].Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
            onstage_charactors[i].Update();
        else /* out of field die with no mercy */
            DisableAll(onstage_charactors[i].ID);
    }

    /* all physics access and modify should dircetly to physical body,
     * that means logic part have full rw access to physical body, while
     * renderer just read some status, these things should not use event
     * system, because they happenned so frequently */

    /* collision detected by physical engine, logic part using callback
     * to handle simple collision (such as get some damage, change status...).
     * this means physical body is intergrate in logic part, callback just call
     * body's specific function with each other's infomation */

    /* Keep Player controlled in field. */
    b2Vec2 pp, pv;
    float dx, dy;
    pp = player->GetPosition();
    pv = player->GetLinearVelocity();
    dx = pp.x + (pv.x * time_step);
    dy = pp.y + (pv.y * time_step);
    if (dx < 0.f)
    {
        pv.x = -pp.x / time_step;
        player->SetLinearVelocity(pv);
    }
    else if (dx > PHYSICAL_WIDTH)
    {
        pv.x = (PHYSICAL_WIDTH - pp.x) / time_step;
        player->SetLinearVelocity(pv);
    }
    if (dy < 0.f)
    {
        pv.y = -pp.y / time_step;
        player->SetLinearVelocity(pv);
    }
    else if (dy > PHYSICAL_HEIGHT)
    {
        pv.y = (PHYSICAL_HEIGHT - pp.y) / time_step;
        player->SetLinearVelocity(pv);
    }

    /* Physic sim, colloisions happen and event happen (damage, state change...).
     * But no post processing, let Lua think the char is turly dead or not. If it is,
     * Lua will tell game to finally disable itself. So dead processing happens very first
     * in loop. */
    world->Step(time_step, VELOCITY_ITERATIONS, POSITION_ITERATIONS);

    /* Stage update. Level event pass in here. New things on stage
     * here, controled by Lua. */
    int rn = 0;
    int good = lua_resume(L_stage, nullptr, 1, &rn);
#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG stage lua error: " << lua_tostring(L_stage, -1) << std::endl;
    if (rn != 0)
        std::cerr << "STG stage lua return something stupid!\n";
#endif
    /* STG game over, just notify game. */
    if (good != LUA_YIELD)
        GameCon->STGReturn(true);

    /* Memory management */
    for (int i = 0; i < disabled_n; i++)
    {
        int flaw = disabled[i];
        switch (disabled_t[i])
        {
        case STGCompType::CHARACTOR:
            onstage_charactors[flaw].CPPSuckSwap(onstage_charactors[charactors_n - 1]);
            records[onstage_charactors[flaw].ID][static_cast<int>(STGCompType::CHARACTOR)] = flaw;
            charactors_n -= 1;
            break;
        case STGCompType::RENDER:
            sprite_renderers[flaw].CPPSuckSwap(sprite_renderers[sprite_renderers_n - 1]);
            records[sprite_renderers[flaw].ID][static_cast<int>(STGCompType::RENDER)] = flaw;
            sprite_renderers_n -= 1;
            break;
        case STGCompType::THINKER:
            onstage_thinkers[flaw].CPPSuckSwap(onstage_thinkers[thinkers_n - 1]);
            records[onstage_thinkers[flaw].ID][static_cast<int>(STGCompType::THINKER)] = flaw;
            thinkers_n -= 1;
            break;
        }
    }
    disabled_n = 0;
}

void STGLevel::Render(float forward_time)
{
#ifndef STG_JUST_DRAW_PHY
    /* draw to off-window frame */
    for (int i = 0; i < sprite_renderers_n; i++)
        sprite_renderers[i].Draw(forward_time);
#endif

#ifdef STG_DEBUG_PHY_DRAW
    world->DrawDebugData();
    p_draw.DrawContact(d_contact_listener.ContactPairs);
#endif
}

/*************************************************************************************************
 *                                                                                               *
 *                                                   Pool                                        *
 *                                                                                               *
 *************************************************************************************************/

/* Some pool control. */
inline int STGLevel::get_id() noexcept
{
    for (int i = record_hint; i < MAX_ON_STAGE; i++)
    {
        if (!used_record[i])
        {
            record_hint = (i + 1) % MAX_ON_STAGE;
            used_record[i] = true;
            return i;
        }
    }

    record_hint = 0;
    for (int i = record_hint; i < MAX_ON_STAGE; i++)
    {
        if (!used_record[i])
        {
            record_hint = (i + 1) % MAX_ON_STAGE;
            used_record[i] = true;
            return i;
        }
    }

    std::cerr << "Too many things on stage!\n";
    return 0;
}

inline void STGLevel::reset_id() noexcept
{
    std::memset(used_record, 0, sizeof(used_record));
    std::memset(records, -1, sizeof(records));
    record_hint = 0;
}

inline void STGLevel::return_id(int id) noexcept
{
    used_record[id] = false;
}

/*************************************************************************************************
 *                                                                                               *
 *                             STGLevel Stage Flow Control API                                   *
 *                                                                                               *
 *************************************************************************************************/

inline void STGLevel::process_pattern_data(SCPatternsCode ptn, SCPatternData &pd) const noexcept
{
    switch (ptn)
    {
    case SCPatternsCode::MOVE_TO:
        pd.Vec.X *= PHYSICAL_WIDTH;
        pd.Vec.Y *= PHYSICAL_HEIGHT;
        break;

    case SCPatternsCode::MOVE_PASSBY:
        for (int i = 0; i < pd.Passby.Num; i++)
        {
            pd.Passby.Vec[i].X *= PHYSICAL_WIDTH;
            pd.Passby.Vec[i].Y *= PHYSICAL_HEIGHT;
        }
    }
}

/* Prepare pattern for it. */
void STGLevel::FillMind(int id, SCPatternsCode ptn, SCPatternData pd) noexcept
{
    process_pattern_data(ptn, pd);
    standby[id].MyPtn = ptn;
    standby[id].MyPD = std::move(pd);
}

void STGLevel::Debut(int id, float x, float y)
{
    int real_id = get_id();

    bd.position.Set(x * PHYSICAL_WIDTH, y * PHYSICAL_HEIGHT);
    b2Body *b = world->CreateBody(&bd);

    if (standby[id].MyPtn != SCPatternsCode::STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_thinkers[thinkers_n].InputMaster);
        al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                                 onstage_charactors[charactors_n].KneeJump);

        onstage_thinkers[thinkers_n].Active(real_id, standby[id].MyPtn, standby[id].MyPD, b);

        records[real_id][static_cast<int>(STGCompType::THINKER)] = thinkers_n;
        thinkers_n += 1;
    }

    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);

    onstage_charactors[charactors_n].Enable(real_id, b,
                                            my_charactor[standby[id].MyChar], standby[id].MyEnter);
    sprite_renderers[sprite_renderers_n].Show(real_id, b,
                                              my_charactor[standby[id].MyChar].Texs.VeryFirstTex);
    records[real_id][static_cast<int>(STGCompType::CHARACTOR)] = charactors_n;
    records[real_id][static_cast<int>(STGCompType::RENDER)] = sprite_renderers_n;
    charactors_n += 1;
    sprite_renderers_n += 1;
}

void STGLevel::Airborne(int id, float x, float y, SCPatternsCode ptn, SCPatternData pd)
{
    int real_id = get_id();

    bd.position.Set(x * PHYSICAL_WIDTH, y * PHYSICAL_HEIGHT);
    b2Body *b = world->CreateBody(&bd);

    if (ptn != SCPatternsCode::STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_thinkers[thinkers_n].InputMaster);
        al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                                 onstage_charactors[charactors_n].KneeJump);

        process_pattern_data(ptn, pd);

        onstage_thinkers[thinkers_n].Active(real_id, ptn, std::move(pd), b);
        records[real_id][static_cast<int>(STGCompType::THINKER)] = thinkers_n;
        thinkers_n += 1;
    }

    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);

    onstage_charactors[charactors_n].Enable(real_id, b, my_charactor[standby[id].MyChar],
                                            all_state.CopyChar(standby[id].MyEnter,
                                                               my_charactor[standby[id].MyChar]));
    sprite_renderers[sprite_renderers_n].Show(real_id, b,
                                              my_charactor[standby[id].MyChar].Texs.VeryFirstTex);
    records[real_id][static_cast<int>(STGCompType::CHARACTOR)] = charactors_n;
    records[real_id][static_cast<int>(STGCompType::RENDER)] = sprite_renderers_n;
    charactors_n += 1;
    sprite_renderers_n += 1;
}

void STGLevel::Pause() const
{
    /* Notify the Game, Game will stop update everything. */
    GameCon->STGPause();
}

/* No charactor, no everything. */
void STGLevel::DisableAll(int id)
{
    int flaw;

    flaw = records[id][static_cast<int>(STGCompType::CHARACTOR)];

    world->DestroyBody(onstage_charactors[flaw].Physics);

    disabled[disabled_n] = flaw;
    disabled_t[disabled_n] = STGCompType::CHARACTOR;
    disabled_n += 1;

    flaw = records[id][static_cast<int>(STGCompType::RENDER)];
    if (flaw > -1)
    {
        al_unregister_event_source(
            sprite_renderers[records[id][static_cast<int>(STGCompType::RENDER)]].Recv,
            onstage_charactors[flaw].RendererMaster);

        disabled[disabled_n] = flaw;
        disabled_t[disabled_n] = STGCompType::RENDER;
        disabled_n += 1;
    }

    if (records[id][static_cast<int>(STGCompType::THINKER)] > -1)
        DisableThr(id);

    records[id][static_cast<int>(STGCompType::CHARACTOR)] = -1;
    records[id][static_cast<int>(STGCompType::RENDER)] = -1;
    return_id(id);
}

void STGLevel::DisableThr(int id)
{
    int flaw = records[id][static_cast<int>(STGCompType::THINKER)];

    al_unregister_event_source(
        onstage_thinkers[flaw].Recv,
        onstage_charactors[records[id][static_cast<int>(STGCompType::CHARACTOR)]].KneeJump);
    al_unregister_event_source(
        onstage_charactors[records[id][static_cast<int>(STGCompType::CHARACTOR)]].InputTerminal,
        onstage_thinkers[flaw].InputMaster);

    disabled[disabled_n] = flaw;
    disabled_t[disabled_n] = STGCompType::THINKER;
    disabled_n += 1;

    records[id][static_cast<int>(STGCompType::THINKER)] = -1;
}