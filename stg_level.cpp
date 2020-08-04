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
    case SCPatternsCode::MOVE_TO:
    case SCPatternsCode::MOVE_LAST:
        data->Vec.X = luaL_checknumber(L, pd_idx + 1);
        data->Vec.Y = luaL_checknumber(L, pd_idx + 2);
        break;
    case SCPatternsCode::MOVE_PASSBY:
        if (!lua_isboolean(L, pd_idx + 1))
            luaL_typeerror(L, pd_idx + 1, "boolean");
        data->Passby.Loop = lua_toboolean(L, pd_idx + 1);
        ;
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
    case SCPatternsCode::MOVE_TO:
    case SCPatternsCode::MOVE_LAST:
        data->Vec.X = lua_tonumber(L, pd_idx + 1);
        data->Vec.Y = lua_tonumber(L, pd_idx + 2);
        break;
    case SCPatternsCode::MOVE_PASSBY:
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
 * 1=level, 2=char_id, 3=coroutine
 * no return */
static int further_load_thinker(lua_State *L)
{
    int id;
    STGLevel *level;

#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    id = luaL_checkinteger(L, 2) - 1;
    luaL_checktype(L, 3, LUA_TTHREAD);
#else
    id = lua_tointeger(L, 2) - 1;
#endif
    level = static_cast<STGLevel *>(lua_touserdata(L, 1));

    level->Brain(id, lua_tothread(L, 3));

    return 0;
}

/* Tell game to further load a charactor. It must debut at sometime. 
 * 1=level, 2=char_id, 3=pattern, 4...=pd
 * no return */
static int further_load_pattern(lua_State *L)
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

    if (!lua_isthread(L, 5))
    {
        load_pattern(L, 5, &ptn, &pd);
        level->Airborne(id, x, y, ptn, std::move(pd));
    }
    else
        level->Airborne(id, x, y, lua_tothread(L, 5));

    return 0;
}

static const luaL_Reg level_con[] = {
    {"further_load_thinker", further_load_thinker},
    {"further_load_pattern", further_load_pattern},
    {"debut", debut},
    {"airborne", airborne},
    {NULL, NULL}};

/*************************************************************************************************
 *                                                                                               *
 *                          STGLevel Initialize / Destroy Function                               *
 *                                                                                               *
 *************************************************************************************************/

void STGLevel::Load(int width, int height, float time_step,
                    const STGLevelSetting &setting, GameFlowController *c)
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
    physical_width = static_cast<float>(width) / (PIXIL_PRE_M * scale);
    physical_height = static_cast<float>(height) / (PIXIL_PRE_M * scale);
    con = c;
    this->width = width;
    this->height = height;
    this->time_step = time_step;
    bound[0] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[1] = physical_height + STG_FIELD_BOUND_BUFFER;
    bound[2] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[3] = physical_width + STG_FIELD_BOUND_BUFFER;
    world = new b2World(b2Vec2(0.f, 0.f));

    /* Set right thing for comps */
    for (int i = 0; i < MAX_ON_STAGE; i++)
    {
        sprite_renderers[i].SetScale(scale, scale, PIXIL_PRE_M * scale);

        onstage_thinkers[i].Con = this;
        onstage_charactors[i].Con = this;
    }
    onstage_patterns[0].Con = this;
    onstage_patterns[1].Con = this;

    /* pool reset */
    reset_id();
    all_state.Clear();

    /* Preload function just load all the resource, like sprite, sound, anime, etc.
     * Some in game feature, like physics, script coroutine, etc. should be initialize
     * in different time: someone in level load phase, someone in stage runtime.
     * Let stage thread first call to initialize whatever, not here. */
    for (size_t i = 0; i < setting.Charactors.size(); i++)
    {
        my_charactor[i] = ResourceManager::GetSTGChar(setting.Charactors[i]);
        my_enter[i] = all_state.MakeChar(my_charactor[i]);

        /* Tune the filter, here we can tishu. */
        my_charactor[i].Phy.FD.filter.groupIndex = static_cast<int16>(CollisionType::G_OTHERS_SIDE);
        my_charactor[i].Phy.FD.filter.categoryBits = static_cast<uint16>(CollisionType::C_ENEMY);
        my_charactor[i].Phy.FD.filter.maskBits = static_cast<uint16>(CollisionType::M_ALL_PLAYER);
    }

    /* Get stage thread in position. (coroutine, function already on the top) */
    L_stage = ResourceManager::GetSTGStageCoroutine(setting.Name);
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
    bd.position.Set(physical_width * .5f, physical_height * .75f);
    player = world->CreateBody(&bd);
    GPlayer.Phy.FD.filter.groupIndex = static_cast<int16>(CollisionType::G_PLAYER_SIDE);
    GPlayer.Phy.FD.filter.categoryBits = static_cast<uint16>(CollisionType::C_PLAYER);
    GPlayer.Phy.FD.filter.maskBits = static_cast<uint16>(CollisionType::M_ALL_ENEMY);
    /* MUST BE 0 */
    int p_id = get_id();
    /* Player on stage. */
    onstage_charactors[0].Enable(p_id, player, GPlayer, all_state.MakeChar(GPlayer));
    sprite_renderers[0].Show(p_id, player, GPlayer.Texs.VeryFirstTex);
    onstage_thinkers[0].Active(p_id, player_watching);
    /* id record */
    records[p_id][static_cast<int>(STGCompType::CHARACTOR)] = 0;
    records[p_id][static_cast<int>(STGCompType::RENDER)] = 0;
    records[p_id][static_cast<int>(STGCompType::THINKER)] = 0;
    /* Update information for update loop. */
    charactors_n = 1;
    thinkers_n = 1;
    sprite_renderers_n = 1;
    patterns_n = 0;
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
     * charactor will notify render... All these comps will tell level to disable themselves. */
    for (int i = thinkers_n - 1; i >= 0; i--)
        onstage_thinkers[i].Think();

    /* Simple pattern do not recieve feedback from charactor, that means they do no thinking or 
     * do reaction. Just do it. */
    for (int i = patterns_n - 1; i >= 0; i--)
        onstage_patterns[i].Update();

    /* Disabel happnned here. Disable state will be set when dead, but not action instantly.
     * Because disable state by "collision", "not in screen" and "stage". The state action will be
     * executed in next loop. So before that, thinker can save. */
    /* Charactors logic, command execute, state update, anime fresh.
     * Stgcharactor just store state and have a hit function for colloision to call. */
    for (int i = charactors_n - 1; i >= 0; i--)
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
    else if (dx > physical_width)
    {
        pv.x = (physical_width - pp.x) / time_step;
        player->SetLinearVelocity(pv);
    }
    if (dy < 0.f)
    {
        pv.y = -pp.y / time_step;
        player->SetLinearVelocity(pv);
    }
    else if (dy > physical_height)
    {
        pv.y = (physical_height - pp.y) / time_step;
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
        con->STGReturn(true);
}

void STGLevel::Render(float forward_time)
{
    /* draw to off-window frame */
    for (int i = 0; i < sprite_renderers_n; i++)
        sprite_renderers[i].Draw(forward_time);

    /* post process */
}

/*************************************************************************************************
 *                                                                                               *
 *                                                   Pool                                        *
 *                                                                                               *
 *************************************************************************************************/

/* Some pool control. */
int STGLevel::get_id() noexcept
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
    std::abort();
    return -1;
}

void STGLevel::reset_id() noexcept
{
    std::memset(used_record, 0, sizeof(used_record));
    std::memset(records, -1, sizeof(records));
    record_hint = 0;
}

void STGLevel::return_id(int id) noexcept
{
    used_record[id] = false;
}

/*************************************************************************************************
 *                                                                                               *
 *                             STGLevel Stage Flow Control API                                   *
 *                                                                                               *
 *************************************************************************************************/

/* Prepare pattern for it. */
void STGLevel::FillMind(int id, SCPatternsCode ptn, SCPatternData pd) noexcept
{
    switch (ptn)
    {
    case SCPatternsCode::MOVE_TO:
        pd.Vec.X *= physical_width;
        pd.Vec.Y *= physical_height;
        break;

    case SCPatternsCode::MOVE_PASSBY:
        pd.Passby.Where = 0;
        for (int i = 0; i < pd.Passby.Num; i++)
        {
            pd.Passby.Vec[i].X *= physical_width;
            pd.Passby.Vec[i].Y *= physical_height;
        }
    }

    my_pattern[id] = ptn;
    my_pattern_data[id] = std::move(pd);
}

/* Prepare thinker for it. Coroutine init call by Lua is done. */
void STGLevel::Brain(int id, lua_State *co) noexcept
{
    my_thinker[id] = co;
}

void STGLevel::Debut(int id, float x, float y)
{
    int real_id = get_id();

    bd.position.Set(x * physical_width, y * physical_height);
    b2Body *b = world->CreateBody(&bd);

    if (my_pattern[id] == SCPatternsCode::CONTROLLED)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_thinkers[thinkers_n].InputMaster);
        al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                                 onstage_charactors[charactors_n].KneeJump);

        onstage_thinkers[thinkers_n].Active(real_id, my_thinker[id]);
        records[real_id][static_cast<int>(STGCompType::THINKER)] = thinkers_n;
        thinkers_n += 1;
    }
    else if (my_pattern[id] != SCPatternsCode::STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_patterns[patterns_n].InputMaster);

        onstage_patterns[patterns_n].Active(real_id, my_pattern[id], my_pattern_data[id], b);
        records[real_id][static_cast<int>(STGCompType::PATTERN)] = patterns_n;
        patterns_n += 1;
    }

    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);

    onstage_charactors[charactors_n].Enable(real_id, b, my_charactor[id], my_enter[id]);
    sprite_renderers[sprite_renderers_n].Show(real_id, b, my_charactor[id].Texs.VeryFirstTex);
    records[real_id][static_cast<int>(STGCompType::CHARACTOR)] = charactors_n;
    records[real_id][static_cast<int>(STGCompType::RENDER)] = sprite_renderers_n;
    charactors_n += 1;
    sprite_renderers_n += 1;
}

void STGLevel::Airborne(int id, float x, float y, lua_State *co)
{
    int real_id = get_id();

    bd.position.Set(x * physical_width, y * physical_height);
    b2Body *b = world->CreateBody(&bd);

    al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                             onstage_thinkers[thinkers_n].InputMaster);
    al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                             onstage_charactors[charactors_n].KneeJump);

    onstage_thinkers[thinkers_n].Active(real_id, co);
    records[real_id][static_cast<int>(STGCompType::THINKER)] = thinkers_n;
    thinkers_n += 1;

    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);

    onstage_charactors[charactors_n].Enable(real_id, b, my_charactor[id],
                                            all_state.MakeChar(my_charactor[id]));
    sprite_renderers[sprite_renderers_n].Show(real_id, b, my_charactor[id].Texs.VeryFirstTex);
    records[real_id][static_cast<int>(STGCompType::CHARACTOR)] = charactors_n;
    records[real_id][static_cast<int>(STGCompType::RENDER)] = sprite_renderers_n;
    charactors_n += 1;
    sprite_renderers_n += 1;
}

void STGLevel::Airborne(int id, float x, float y, SCPatternsCode ptn, SCPatternData pd)
{
    int real_id = get_id();

    bd.position.Set(x * physical_width, y * physical_height);
    b2Body *b = world->CreateBody(&bd);

    if (ptn != SCPatternsCode::STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_patterns[patterns_n].InputMaster);

        onstage_patterns[patterns_n].Active(real_id, ptn, std::move(pd), b);
        records[real_id][static_cast<int>(STGCompType::PATTERN)] = patterns_n;
        patterns_n += 1;
    }

    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);

    onstage_charactors[charactors_n].Enable(real_id, b, my_charactor[id],
                                            all_state.MakeChar(my_charactor[id]));
    sprite_renderers[sprite_renderers_n].Show(real_id, b, my_charactor[id].Texs.VeryFirstTex);
    records[real_id][static_cast<int>(STGCompType::CHARACTOR)] = charactors_n;
    records[real_id][static_cast<int>(STGCompType::RENDER)] = sprite_renderers_n;
    charactors_n += 1;
    sprite_renderers_n += 1;
}

void STGLevel::Pause() const
{
    /* Notify the Game, Game will stop update everything. */
    con->STGPause();
}

void STGLevel::DisableAll(int id)
{
    int flaw = records[id][static_cast<int>(STGCompType::CHARACTOR)];

    world->DestroyBody(
        onstage_charactors[flaw].Physics);

    if (records[id][static_cast<int>(STGCompType::THINKER)] > -1)
        DisableThr(id);
    if (records[id][static_cast<int>(STGCompType::PATTERN)] > -1)
        DisablePtn(id);

    if (records[id][static_cast<int>(STGCompType::RENDER)] > -1)
        al_unregister_event_source(
            sprite_renderers[records[id][static_cast<int>(STGCompType::RENDER)]].Recv,
            onstage_charactors[flaw].RendererMaster);

    onstage_charactors[flaw].CPPSuckSwap(onstage_charactors[charactors_n - 1]);
    records[onstage_charactors[flaw].ID][static_cast<int>(STGCompType::CHARACTOR)] = flaw;
    charactors_n -= 1;

    flaw = records[id][static_cast<int>(STGCompType::RENDER)];
    sprite_renderers[flaw].CPPSuckSwap(sprite_renderers[sprite_renderers_n - 1]);
    records[sprite_renderers[flaw].ID][static_cast<int>(STGCompType::RENDER)] = flaw;
    sprite_renderers_n -= 1;

    records[id][static_cast<int>(STGCompType::CHARACTOR)] = -1;
    records[id][static_cast<int>(STGCompType::RENDER)] = -1;
    return_id(id);
}

void STGLevel::DisablePtn(int id)
{
    int flaw = records[id][static_cast<int>(STGCompType::PATTERN)];

    al_unregister_event_source(
        onstage_charactors[records[id][static_cast<int>(STGCompType::CHARACTOR)]].InputTerminal,
        onstage_patterns[flaw].InputMaster);

    onstage_patterns[flaw].CPPSuckSwap(onstage_patterns[patterns_n - 1]);
    records[onstage_patterns[flaw].ID][static_cast<int>(STGCompType::PATTERN)] = flaw;
    patterns_n -= 1;

    records[id][static_cast<int>(STGCompType::PATTERN)] = -1;
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

    onstage_thinkers[flaw].CPPSuckSwap(onstage_thinkers[thinkers_n - 1]);
    records[onstage_thinkers[flaw].ID][static_cast<int>(STGCompType::THINKER)] = flaw;
    thinkers_n -= 1;

    records[id][static_cast<int>(STGCompType::THINKER)] = -1;
}