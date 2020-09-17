#include "stg_level.h"

#include "resource_manger.h"
#include "llauxlib.h"

#include <allegro5/allegro_primitives.h>

#include <unordered_map>
#include <iostream>
#include <queue>

/*************************************************************************************************
 *                                                                                               *
 *                                     Lua Commands API                                          *
 *                                                                                               *
 *************************************************************************************************/

/* Tell game to further load a charactor. It must debut at sometime. 
 * 1=level, 2=char_id, 3=pattern, 4...=pd
 * no return */
static int further_load(lua_State *L)
{
    int id;
    SCPatternData pd;
    STGLevel *level;

#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
    id = luaL_checkinteger(L, 2) - 1;
#else
    id = lua_tointeger(L, 2) - 1;
#endif
    level = reinterpret_cast<STGLevel *>(lua_touserdata(L, 1));

    SCPatternsCode ptn = use_sc_pattern(L, 3, pd);
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
    level = reinterpret_cast<STGLevel *>(lua_touserdata(L, 1));

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
    level = reinterpret_cast<STGLevel *>(lua_touserdata(L, 1));

    SCPatternsCode ptn = use_sc_pattern(L, 5, pd);
    level->Airborne(id, x, y, ptn, std::move(pd));

    return 0;
}

/* Tell game play is winner.
 * 1=level
 * no return */
int player_win(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    reinterpret_cast<STGLevel *>(lua_touserdata(L, 1))->PlayerWin();

    return 0;
}

int player_dying(lua_State *L)
{
#ifdef STG_LUA_API_ARG_CHECK
    luaL_checktype(L, 1, LUA_TLIGHTUSERDATA);
#endif

    reinterpret_cast<STGLevel *>(lua_touserdata(L, 1))->PlayerDying();

    return 0;
}

static const luaL_Reg level_con[] = {
    {"further_load", further_load},
    {"debut", debut},
    {"airborne", airborne},
    {"player_win", player_win},
    {"player_dying", player_dying},
    {NULL, NULL}};

/*************************************************************************************************
 *                                                                                               *
 *                          STGLevel Initialize / Destroy Function                               *
 *                                                                                               *
 *************************************************************************************************/

STGLevel::STGLevel()
{
    /* Body define can be share, all charactors is kinematic. */
    bd.type = b2_kinematicBody;
    bd.fixedRotation = true;

    for (int i = 0; i < MAX_ON_STAGE; i++)
    {
        onstage_thinkers[i].Con = this;
        onstage_charactors[i].Con = this;
    }
    for (int i = 0; i < MAX_ENTITIES * 2; i++)
        many_shooters[i].Con = this;
    for (int i = 0; i < MAX_ENTITIES / 2; i++)
        bullets[i].Con = this;

    count_down = al_create_timer(1.);
    if (!count_down)
    {
        std::cerr << "Failed to initialize STG Level's count down timer!\n";
        std::abort();
    }
}

STGLevel::~STGLevel()
{
    al_destroy_timer(count_down);
}

void STGLevel::Load(const STGLevelSetting &setting)
{
#ifdef STG_PERFORMENCE_SHOW
    tr_bullets_n.Text = {reinterpret_cast<const char *>(u8"Number of bullets: "),
                         ResourceManager::GetFont("hana_a_10"), 10.f, 10.f, ALLEGRO_ALIGN_LEFT};
    tr_bn.Text = {"", ResourceManager::GetFont("hana_a_10"),
                  tr_bullets_n.GetRight(), 10.f, ALLEGRO_ALIGN_LEFT};
#endif

    timer = 0;

    Name = setting.Name;
    CodeName = setting.CodeName;

    this->time_step = g_time_step;
    bound[0] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[1] = PHYSICAL_HEIGHT + STG_FIELD_BOUND_BUFFER;
    bound[2] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[3] = PHYSICAL_WIDTH + STG_FIELD_BOUND_BUFFER;
    world = new b2World(b2Vec2(0.f, 0.f));
    world->SetContactListener(&contact_listener);

#ifdef STG_DEBUG_PHY_DRAW
    p_draw.Init(PIXIL_PRE_M);
    world->SetDebugDraw(&p_draw);
#endif

    /* pool reset */
    reset_id();
    shooters_p = nullptr;
    shooters_n = 0;
    bullets_p = nullptr;
    bullets_n = 0;
    all_state.Reset();
    charactors_n = 0;
    thinkers_n = 0;
    sprite_renderers_n = 0;

    /* Recording which bullet has been loaded. */
    b2Filter f;
    std::unordered_map<std::string, Bullet *> loaded_bullets;

    /* Preload player */
    int player_shooters_n = 0;
    GPlayer.MyChar = ResourceManager::GetSTGChar(SPlayer.Char);
    GPlayer.MyShooters = many_shooters;
    /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
    GPlayer.MyChar.Phy.FD.shape = GPlayer.MyChar.Phy.Shape == ShapeType::ST_CIRCLE
                                      ? static_cast<b2Shape *>(&GPlayer.MyChar.Phy.C)
                                      : static_cast<b2Shape *>(&GPlayer.MyChar.Phy.P);
    GPlayer.MyChar.Phy.FD.filter.groupIndex = CollisionType::G_PLAYER_SIDE;
    GPlayer.MyChar.Phy.FD.filter.categoryBits = CollisionType::C_PLAYER;
    GPlayer.MyChar.Phy.FD.filter.maskBits = CollisionType::M_ALL_ENEMY;
    std::queue<Bullet *> bss;
    f.groupIndex = CollisionType::G_PLAYER_SIDE;
    f.categoryBits = CollisionType::C_PLAYER_BULLET;
    f.maskBits = CollisionType::C_ENEMY;
    for (const auto &bn : SPlayer.Bulletss)
    {
        if (!loaded_bullets.contains(bn))
        {
            bullets[bullets_n].Load(ResourceManager::GetSTGBullet(bn), f, world);
            loaded_bullets.emplace(bn, bullets + bullets_n);
            bullets_n += 1;
        }

        bss.push(loaded_bullets[bn]);
    }
    for (const auto &sn : SPlayer.Shooters)
    {
        many_shooters[shooters_n + player_shooters_n].Load(bound,
                                                           ResourceManager::GetSTGShooter(sn),
                                                           bss);
        many_shooters[shooters_n + player_shooters_n].MyDearPlayer();
        many_shooters[shooters_n + player_shooters_n].ShiftDown =
            many_shooters + shooters_n + player_shooters_n + 1;
        many_shooters[shooters_n + player_shooters_n].ShiftUp =
            many_shooters + shooters_n + player_shooters_n - 1;
        player_shooters_n += 1;
    }
    if (player_shooters_n > 0)
    {
        GPlayer.MyShooters = many_shooters + shooters_n;
        /* Shooters shift in a circle. */
        many_shooters[shooters_n + player_shooters_n - 1].ShiftDown = many_shooters + shooters_n;
        many_shooters[shooters_n].ShiftUp = many_shooters + shooters_n + player_shooters_n - 1;
    }
    shooters_n += player_shooters_n;
    GPlayer.MyEnter = all_state.MakeChar(GPlayer.MyChar.Texs);

    /* Preload function just load all the resource, like sprite, sound, anime, etc.
     * Some in game feature, like physics, script coroutine, etc. should be initialize
     * in different time: someone in level load phase, someone in stage runtime.
     * Let stage thread first call to initialize whatever, not here. */
    f.groupIndex = CollisionType::G_ENEMY_SIDE;
    f.categoryBits = CollisionType::C_ENEMY_BULLET;
    f.maskBits = CollisionType::C_PLAYER;

    assert(setting.Charactors.size() <= MAX_ENTITIES);

    for (size_t i = 0; i < setting.Charactors.size(); i++)
    {
        standby[i].MyChar = ResourceManager::GetSTGChar(setting.Charactors[i].Char);

        /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
        standby[i].MyChar.Phy.FD.shape = standby[i].MyChar.Phy.Shape == ShapeType::ST_CIRCLE
                                             ? static_cast<b2Shape *>(&standby[i].MyChar.Phy.C)
                                             : static_cast<b2Shape *>(&standby[i].MyChar.Phy.P);
        /* Tune the filter, here we can tishu. */
        standby[i].MyChar.Phy.FD.filter.groupIndex = CollisionType::G_ENEMY_SIDE;
        standby[i].MyChar.Phy.FD.filter.categoryBits = CollisionType::C_ENEMY;
        standby[i].MyChar.Phy.FD.filter.maskBits = CollisionType::M_ALL_PLAYER;

        /* Make shooter */
        standby[i].MyShooters = nullptr;
        int my_shooters_n = 0;
        std::queue<Bullet *> bss;
        for (const auto &bn : setting.Charactors[i].Bulletss)
        {
            if (!loaded_bullets.contains(bn))
            {
                bullets[bullets_n].Load(ResourceManager::GetSTGBullet(bn), f, world);
                loaded_bullets.emplace(bn, bullets + bullets_n);
                bullets_n += 1;
                assert(bullets_n <= MAX_ENTITIES / 2);
            }

            bss.push(loaded_bullets[bn]);
        }
        for (const auto &sn : setting.Charactors[i].Shooters)
        {
            many_shooters[shooters_n + my_shooters_n].Load(bound,
                                                           ResourceManager::GetSTGShooter(sn),
                                                           bss);
            many_shooters[shooters_n + my_shooters_n].ShiftDown =
                many_shooters + shooters_n + my_shooters_n + 1;
            many_shooters[shooters_n + my_shooters_n].ShiftUp =
                many_shooters + shooters_n + my_shooters_n - 1;
            my_shooters_n += 1;
        }
        if (my_shooters_n > 0)
        {
            standby[i].MyShooters = &many_shooters[shooters_n];
            /* Shooters shift in a circle. */
            many_shooters[shooters_n + my_shooters_n - 1].ShiftDown = many_shooters + shooters_n;
            many_shooters[shooters_n].ShiftUp = many_shooters + shooters_n + my_shooters_n - 1;
        }
        shooters_n += my_shooters_n;

        assert(shooters_n <= MAX_ENTITIES * 2);

        /* Make state */
        standby[i].MyEnter = all_state.MakeChar(standby[i].MyChar.Texs);
    }

    /* Get stage thread in position. (coroutine, function already on the top) */
    L_stage = ResourceManager::GetCoroutine(ResourceManager::STG_STAGE_FUNCTIONS_KEY,
                                            setting.CodeName);
    /* Initialize call. thinker lib, Level lib and level light userdata passed in. 
     * Player watching will be return. */
    int rn = 1;
    lua_State *player_watching;
    STGThinker::HandoverController(L_stage);
    luaL_newlib(L_stage, level_con);
    lua_pushlightuserdata(L_stage, this);
    lua_pushinteger(L_stage, UPDATE_PER_SEC);
    /* So here the stage thread first call will tell game to further init some charactors
     * who can be compelet initialized at this phase (mainly thinker). */
#ifdef STG_LUA_API_ARG_CHECK
    int good = lua_resume(L_stage, nullptr, 4, &rn);
    if (good != LUA_YIELD)
        std::cerr << "STG stage Lua preload error: "
                  << (good == LUA_OK ? "it just return." : lua_tostring(L_stage, -1))
                  << std::endl;
    if (!lua_isthread(L_stage, -1))
        std::cerr << "STG stage Lua did not return player watching!\n";
#else
    lua_resume(L_stage, nullptr, 4, &rn);
#endif
    player_watching = lua_tothread(L_stage, -1);
    lua_pop(L_stage, rn);

    /* Connect player's comps. */
    al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                             onstage_thinkers[thinkers_n].InputMaster);
    al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                             onstage_charactors[charactors_n].RendererMaster);
    al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                             onstage_charactors[charactors_n].KneeJump);
    /* Make body and tune fixture for player */
    bd.position.Set(PHYSICAL_WIDTH * .5f, PHYSICAL_HEIGHT * .75f);
    player = world->CreateBody(&bd);
    player->CreateFixture(&GPlayer.MyChar.Phy.FD);
    /* MUST BE 0 */
    int p_id = get_id();
    /* Player on stage. */
    SCPatternData pd;
    pd.ai = player_watching;
    onstage_charactors[charactors_n].Enable(p_id, GPlayer.MyChar, player,
                                            GPlayer.MyShooters, GPlayer.MyEnter);
    sprite_renderers[sprite_renderers_n].Show(p_id, player, GPlayer.MyChar.Texs.VeryFirstTex);
    onstage_thinkers[thinkers_n].Active(p_id, SCPatternsCode::SCPC_CONTROLLED, std::move(pd),
                                        player);
    Shooter *sp = GPlayer.MyShooters;
    do
        sp = sp->Undershift(player);
    while (sp != GPlayer.MyShooters);
    /* Export player's input terminal. */
    PlayerInputTerminal = onstage_charactors[charactors_n].InputTerminal;
    /* id record */
    records[p_id][STGCompType::SCT_CHARACTOR] = charactors_n;
    records[p_id][STGCompType::SCT_RENDER] = thinkers_n;
    records[p_id][STGCompType::SCT_THINKER] = sprite_renderers_n;
    /* Update information for update loop. */
    charactors_n += 1;
    thinkers_n += 1;
    sprite_renderers_n += 1;

    /* set right draw */
    ALLEGRO_TRANSFORM ts;
    float inner_scale =
        (static_cast<float>(SCREEN_HEIGHT) - FRAME_TKN * 2.f) / static_cast<float>(SCREEN_HEIGHT);
    al_identity_transform(&T);
    al_identity_transform(&TC);
    al_translate_transform(&TC, g_offset + 1.f, 0.f);
    al_translate_transform(&TC, FRAME_TKN * g_scale, FRAME_TKN * g_scale);
    al_identity_transform(&ts);
    al_scale_transform(&ts, inner_scale, inner_scale);
    al_scale_transform(&ts, g_scale, g_scale);

    al_set_new_bitmap_flags(ALLEGRO_NO_PRESERVE_TEXTURE | ALLEGRO_VIDEO_BITMAP);
    cover = al_create_bitmap(std::lroundf(static_cast<float>(SCREEN_WIDTH) *
                                          inner_scale * g_scale),
                             std::lroundf(static_cast<float>(SCREEN_HEIGHT) *
                                          inner_scale * g_scale));
    if (cover == nullptr)
    {
        std::cerr << "Failed to initialize STG Level's cover bitmap!\n";
        std::abort();
    }

    al_set_target_bitmap(cover);
    al_use_transform(&ts);
    g_set_orignal_bitmap();
}

/* ONLY call by outside. */
void STGLevel::Unload()
{
    al_destroy_bitmap(cover);
    delete world;
}

/*************************************************************************************************
 *                                                                                               *
 *                          STGLevel    Update    Function                                       *
 *                                                                                               *
 *************************************************************************************************/

void STGLevel::Update()
{
    _ASSERTE(_CrtCheckMemory());

    timer += 1;
    lua_pushinteger(L_stage, timer);
    /* Stage update. Level event pass in here. New things on stage
     * here, controled by Lua. */
    int rn = 0;
    int good = lua_resume(L_stage, nullptr, 1, &rn);
#ifdef STG_LUA_API_ARG_CHECK
    if (good != LUA_OK && good != LUA_YIELD)
        std::cerr << "STG stage lua error: " << lua_tostring(L_stage, -1) << std::endl;
#endif
    /* STG game over, just notify game. */
    if (good != LUA_YIELD || al_get_timer_count(count_down) == 5)
    {
        al_stop_timer(count_down);
        al_set_timer_count(count_down, 0);
        GameCon->STGReturn(true);
        return;
    }

    _ASSERTE(_CrtCheckMemory());

    /* Thinking... Disable has not been submit to flow controller can be cancel here. */
    /* Disable can be invoked by AI thinker (I think I am dead...). Thinker will notify charactor,
     * charactor will notify render... All these comps will tell level to disable themselves.
     * While simple pattern do not recieve feedback from charactor, that means they do no thinking
     * or do reaction. So they do not save disable. */
    for (int i = 0; i < thinkers_n; i++)
        onstage_thinkers[i].Think();

    _ASSERTE(_CrtCheckMemory());

    /* Disable happnned here. Disable state will be set when dead, but not action instantly.
     * Because dead by "collision" in physical world step, the state action will be
     * executed in next loop. So before that, thinker can save.
     * Charactors logic, command execute, state update, anime fresh.
     * Stgcharactor just store state and have a hit function for colloision to call. */
    for (int i = 0; i < charactors_n; i++)
    {
        const b2Vec2 &p = onstage_charactors[i].Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
            onstage_charactors[i].Update();
        else /* out of field die with no mercy */
        {
#ifdef _DEBUG
            std::cout << "Char-" << onstage_charactors[i].CodeName << " becomes outsider!\n";
#endif

            onstage_charactors[i].Farewell();
        }
    }

    _ASSERTE(_CrtCheckMemory());

    /* Disable Execution */
    for (int i = 0; i < disabled_n; i++)
    {
        int flaw = 0;
        int id = disabled[i];

        switch (disabled_t[i])
        {
        case STGCompType::SCT_CHARACTOR:
            flaw = records[id][STGCompType::SCT_CHARACTOR];
            records[id][STGCompType::SCT_CHARACTOR] = -1;

#ifdef _DEBUG
            std::cout << "Charactor final disable: "
                      << id << "-" << onstage_charactors[flaw].CodeName << ".\n";
#endif

            world->DestroyBody(onstage_charactors[flaw].Physics);

            if (flaw + 1 != charactors_n)
            {
                onstage_charactors[flaw].CPPSuckSwap(onstage_charactors[charactors_n - 1]);
                records[onstage_charactors[flaw].ID][STGCompType::SCT_CHARACTOR] = flaw;
            }
            charactors_n -= 1;

            return_id(id);
            break;

        case STGCompType::SCT_RENDER:
            flaw = records[id][STGCompType::SCT_RENDER];
            records[id][STGCompType::SCT_RENDER] = -1;

#ifdef _DEBUG
            std::cout << "Render final disable: " << id << ".\n";
#endif

            if (flaw + 1 != sprite_renderers_n)
            {
                sprite_renderers[flaw].CPPSuckSwap(sprite_renderers[sprite_renderers_n - 1]);
                records[sprite_renderers[flaw].ID][STGCompType::SCT_RENDER] = flaw;
            }
            sprite_renderers_n -= 1;
            break;

        case STGCompType::SCT_THINKER:
            flaw = records[id][STGCompType::SCT_THINKER];
            records[id][STGCompType::SCT_THINKER] = -1;

#ifdef _DEBUG
            std::cout << "Thinker final disable: " << id << ".\n";
#endif

            if (flaw + 1 != thinkers_n)
            {
                onstage_thinkers[flaw].CPPSuckSwap(onstage_thinkers[thinkers_n - 1]);
                records[onstage_thinkers[flaw].ID][STGCompType::SCT_THINKER] = flaw;
            }
            thinkers_n -= 1;
            break;
        }
    }
    disabled_n = 0;

    _ASSERTE(_CrtCheckMemory());

    /* Shooting & Bullet management */
#ifdef _DEBUG
    int lock_free = 0;
#endif

    Shooter *sp = shooters_p;
    while (sp != nullptr)
    {
#ifdef _DEBUG
        lock_free += 1;
        if (lock_free > 100)
        {
            std::cerr << "Shooters list was locked!\n";
            Pause();
            return;
        }
#endif

        sp = sp->Update();
    }

#ifdef STG_PERFORMENCE_SHOW
    int i_bn = 0;
#endif

#ifdef _DEBUG
    lock_free = 0;
#endif

    Bullet *b = bullets_p;
    while (b != nullptr)
    {
#ifdef _DEBUG
        lock_free += 1;
        if (lock_free > 100)
        {
            std::cerr << "Bullet list was locked!\n";
            Pause();
            return;
        }
#endif

#ifdef STG_PERFORMENCE_SHOW
        i_bn += b->GetBulletNum();
#endif

        b = b->Update();
    }

    _ASSERTE(_CrtCheckMemory());

#ifdef STG_PERFORMENCE_SHOW
    tr_bn.ChangeText(i_bn);
#endif

    /* all physics access and modify should dircetly to physical body,
     * that means logic part have full rw access to physical body, while
     * renderer just read some status, these things should not use event
     * system, because they happenned so frequently */

    /* collision detected by physical engine, logic part using callback
     * to handle simple collision (such as get some damage, change status...).
     * this means physical body is intergrate in logic part, callback just call
     * body's specific function with each other's infomation */

    /* Keep Player controlled in field. */
    if (player != nullptr)
    {
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
    }

    /* Physic sim, colloisions happen and event happen (damage, state change...).
     * But no post processing, let Lua think the char is turly dead or not. If it is,
     * Lua will tell game to finally disable itself. So dead processing happens very first
     * in loop. */
    world->Step(time_step, VELOCITY_ITERATIONS, POSITION_ITERATIONS);
}

void STGLevel::Render(float forward_time)
{
    static const ALLEGRO_COLOR OLIVE_GREEN = al_map_rgb(128, 128, 0);
    static const ALLEGRO_COLOR ORANGE = al_map_rgb(211, 211, 211);
    static const ALLEGRO_COLOR BLACK = al_map_rgb(25, 25, 112);

    al_set_target_bitmap(cover);
    al_clear_to_color(BLACK);

#ifndef STG_JUST_DRAW_PHY
    for (int i = 0; i < sprite_renderers_n; i++)
        sprite_renderers[i].Draw(forward_time);

    Bullet *b = bullets_p;
    while (b != nullptr)
        b = b->Draw(forward_time);
#endif

#ifdef STG_DEBUG_PHY_DRAW
    world->DrawDebugData();

    Shooter *sp = shooters_p;
    while (sp != nullptr)
        sp = sp->DrawDebugData();
#endif

    g_set_orignal_bitmap();

    al_use_transform(&T);
    al_draw_filled_rectangle(g_offset, 0.f, g_oppo_offset, static_cast<float>(g_height), ORANGE);
    al_draw_filled_rectangle(0.f, 0.f, g_offset, static_cast<float>(g_height), OLIVE_GREEN);
    al_draw_filled_rectangle(g_oppo_offset, 0.f,
                             static_cast<float>(g_width), static_cast<float>(g_height),
                             OLIVE_GREEN);

    al_use_transform(&TC);
    al_draw_bitmap(cover, 0.f, 0.f, 0);

#ifdef STG_PERFORMENCE_SHOW
    tr_bullets_n.Draw();
    tr_bn.Draw();
#endif
}

const b2Body *STGLevel::TrackEnemy() const noexcept
{
    const b2Vec2 pp = player->GetPosition();
    const b2Body *ret = nullptr;
    float closest_distance_sq = 2000.f * 2000.f; /* Most box2d can do efficiency. */

    for (int i = 1; i < charactors_n; i++)
        /* When charactor is actully dying, 
         * she will still need body to represent her untill disable animation done.
         * While body will be destroyed when dead. 
         * So use this to determine if charactor can be collision to make damage. */
        if (onstage_charactors[i].Physics->IsActive())
        {
            float dis_sq = (onstage_charactors[i].Physics->GetPosition() - pp).LengthSquared();
            if (dis_sq < closest_distance_sq)
            {
                ret = onstage_charactors[i].Physics;
                closest_distance_sq = dis_sq;
            }
        }

    return ret;
}

const b2Body *STGLevel::TrackPlayer() const noexcept
{
    return player;
}

/*************************************************************************************************
 *                                                                                               *
 *                                             ID Pool                                           *
 *                                                                                               *
 *************************************************************************************************/

inline int STGLevel::get_id() noexcept
{
    for (int i = record_hint; i < MAX_ON_STAGE; i++)
    {
        if (!used_record[i])
        {
            record_hint = (i + 1) % MAX_ON_STAGE;
            used_record[i] = true;

#ifdef _DEBUG
            std::cout << "ID get: " << i << ".\n";
#endif
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

#ifdef _DEBUG
            std::cout << "ID get: " << i << ".\n";
#endif
            return i;
        }
    }

    assert(false);
    return 0x0fffffff;
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

#ifdef _DEBUG
    std::cout << "ID returned: " << id << ".\n";
#endif
}

/*************************************************************************************************
 *                                                                                               *
 *                             STGLevel Stage Flow Control API                                   *
 *                                                                                               *
 *************************************************************************************************/

/* Prepare pattern for it. */
void STGLevel::FillMind(int id, SCPatternsCode ptn, SCPatternData pd) noexcept
{
    standby[id].MyPtn = ptn;
    standby[id].MyPD = std::move(pd);
}

void STGLevel::Debut(int id, float x, float y)
{
#ifdef _DEBUG
    std::cout << "Debut: " << standby[id].MyChar.CodeName << ".\n";
#endif

    int real_id = get_id();

    bd.position.Set(x * PHYSICAL_WIDTH, y * PHYSICAL_HEIGHT);
    b2Body *b = world->CreateBody(&bd);
    /* Attach fixture, so char can be collsion. */
    if (standby[id].MyChar.Phy.Physical)
        b->CreateFixture(&standby[id].MyChar.Phy.FD);

    /* Thinker */
    if (standby[id].MyPtn != SCPatternsCode::SCPC_STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_thinkers[thinkers_n].InputMaster);
        al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                                 onstage_charactors[charactors_n].KneeJump);

        onstage_thinkers[thinkers_n].Active(real_id, standby[id].MyPtn, standby[id].MyPD, b);

        records[real_id][STGCompType::SCT_THINKER] = thinkers_n;
        thinkers_n += 1;
    }

    /* Shooter */
    if (standby[id].MyShooters != nullptr)
    {
        Shooter *sp = standby[id].MyShooters;
        do
            sp = sp->Undershift(b);
        while (sp != standby[id].MyShooters);
    }

    /* Renderer */
    if (standby[id].MyChar.Texs.VeryFirstTex.Sprite != nullptr)
    {
        sprite_renderers[sprite_renderers_n].Show(real_id, b,
                                                  standby[id].MyChar.Texs.VeryFirstTex);
        al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                                 onstage_charactors[charactors_n].RendererMaster);
        records[real_id][STGCompType::SCT_RENDER] = sprite_renderers_n;
        sprite_renderers_n += 1;
    }

    onstage_charactors[charactors_n].Enable(real_id, standby[id].MyChar, b,
                                            standby[id].MyShooters, standby[id].MyEnter);
    records[real_id][STGCompType::SCT_CHARACTOR] = charactors_n;
    charactors_n += 1;

    assert(charactors_n <= MAX_ON_STAGE);
    assert(sprite_renderers_n <= MAX_ON_STAGE);
    assert(thinkers_n <= MAX_ON_STAGE);
}

void STGLevel::Airborne(int id, float x, float y, SCPatternsCode ptn, SCPatternData pd)
{
#ifdef _DEBUG
    std::cout << "Airborne: " << standby[id].MyChar.CodeName << ".\n";
#endif

    int real_id = get_id();

    bd.position.Set(x * PHYSICAL_WIDTH, y * PHYSICAL_HEIGHT);
    b2Body *b = world->CreateBody(&bd);
    /* Attach fixture, so char can be collsion. */
    if (standby[id].MyChar.Phy.Physical)
        b->CreateFixture(&standby[id].MyChar.Phy.FD);

    /* Thinker */
    if (ptn != SCPatternsCode::SCPC_STAY)
    {
        al_register_event_source(onstage_charactors[charactors_n].InputTerminal,
                                 onstage_thinkers[thinkers_n].InputMaster);
        al_register_event_source(onstage_thinkers[thinkers_n].Recv,
                                 onstage_charactors[charactors_n].KneeJump);

        onstage_thinkers[thinkers_n].Active(real_id, ptn, std::move(pd), b);
        records[real_id][STGCompType::SCT_THINKER] = thinkers_n;
        thinkers_n += 1;
    }

    /* Shooter */
    Shooter *my_shooters = nullptr;
    if (standby[id].MyShooters != nullptr)
    {
        my_shooters = copy_shooters(standby[id].MyShooters);
        Shooter *sp = my_shooters;
        do
            sp = sp->Undershift(b);
        while (sp != my_shooters);
    }

    /* Renderer */
    if (standby[id].MyChar.Texs.VeryFirstTex.Sprite != nullptr)
    {
        sprite_renderers[sprite_renderers_n].Show(real_id, b, standby[id].MyChar.Texs.VeryFirstTex);
        al_register_event_source(sprite_renderers[sprite_renderers_n].Recv,
                                 onstage_charactors[charactors_n].RendererMaster);
        records[real_id][STGCompType::SCT_RENDER] = sprite_renderers_n;
        sprite_renderers_n += 1;
    }

    /* Charactor, on stage things must contain characotrs (expect bullet). */
    onstage_charactors[charactors_n].Enable(real_id, standby[id].MyChar, b, my_shooters,
                                            all_state.CopyChar(standby[id].MyEnter,
                                                               standby[id].MyChar.Texs));
    records[real_id][STGCompType::SCT_CHARACTOR] = charactors_n;
    charactors_n += 1;

    assert(charactors_n <= MAX_ON_STAGE);
    assert(sprite_renderers_n <= MAX_ON_STAGE);
    assert(thinkers_n <= MAX_ON_STAGE);
}

void STGLevel::Pause() const
{
    /* Notify the Game, Game will stop update everything. */
    GameCon->STGPause();
}

void STGLevel::DisableAll(int id)
{
#ifdef _DEBUG
    std::cout << "Char-" << onstage_charactors[records[id][STGCompType::SCT_CHARACTOR]].CodeName
              << " ID: " << id << " will be disabled!\n";
#endif

    disabled[disabled_n] = id;
    disabled_t[disabled_n] = STGCompType::SCT_CHARACTOR;
    disabled_n += 1;

    if (records[id][STGCompType::SCT_RENDER] > -1)
    {
        al_unregister_event_source(
            sprite_renderers[records[id][STGCompType::SCT_RENDER]].Recv,
            onstage_charactors[records[id][STGCompType::SCT_CHARACTOR]].RendererMaster);

        disabled[disabled_n] = id;
        disabled_t[disabled_n] = STGCompType::SCT_RENDER;
        disabled_n += 1;
    }

    if (records[id][STGCompType::SCT_THINKER] > -1)
        DisableThr(id);
}

void STGLevel::DisableThr(int id)
{
    al_unregister_event_source(
        onstage_thinkers[records[id][STGCompType::SCT_THINKER]].Recv,
        onstage_charactors[records[id][STGCompType::SCT_CHARACTOR]].KneeJump);
    al_unregister_event_source(
        onstage_charactors[records[id][STGCompType::SCT_CHARACTOR]].InputTerminal,
        onstage_thinkers[records[id][STGCompType::SCT_THINKER]].InputMaster);

    disabled[disabled_n] = id;
    disabled_t[disabled_n] = STGCompType::SCT_THINKER;
    disabled_n += 1;

#ifdef _DEBUG
    std::cout << "Thinker will disabled for " << id << ".\n";
#endif
}

/* Only called from shooter inside (in stage). */
void STGLevel::EnableSht(Shooter *ss) noexcept
{
    if (shooters_p != nullptr)
        shooters_p->Prev = ss;
    ss->Next = shooters_p;
    ss->Prev = nullptr;
    shooters_p = ss;

#ifdef _DEBUG
    std::cout << "Shooter enabled: " << ss->CodeName << ".\n";
#endif

#ifdef STG_DEBUG_PHY_DRAW
    ss->DebugDraw = &p_draw;
#endif
}

/* Only called from shooter inside (in stage). */
void STGLevel::DisableSht(Shooter *ss) noexcept
{
    /* Different with others. List just lost, no recycle. */
    if (ss->Prev == nullptr)
        shooters_p = ss->Next;
    else
        ss->Prev->Next = ss->Next;

    if (ss->Next != nullptr)
        ss->Next->Prev = ss->Prev;

#ifdef _DEBUG
    std::cout << "Shooter disabled: " << ss->CodeName << ".\n";
#endif
}

/* Aux Functions */
Shooter *STGLevel::copy_shooters(const Shooter *first)
{
    int copied_shooters_n = 0;
    const Shooter *sp = first;
    Shooter *ret = many_shooters + shooters_n;

    do
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << sp->CodeName << " be copied.\n";
#endif

        many_shooters[shooters_n + copied_shooters_n] = *sp;
        many_shooters[shooters_n + copied_shooters_n].ShiftDown =
            many_shooters + shooters_n + copied_shooters_n + 1;
        many_shooters[shooters_n + copied_shooters_n].ShiftUp =
            many_shooters + shooters_n + copied_shooters_n - 1;
        sp = sp->ShiftDown;
        copied_shooters_n += 1;
    } while (sp != first);
    many_shooters[shooters_n + copied_shooters_n - 1].ShiftDown = many_shooters + shooters_n;
    many_shooters[shooters_n].ShiftUp = many_shooters + shooters_n + copied_shooters_n - 1;

    shooters_n += copied_shooters_n;

    assert(shooters_n <= MAX_ENTITIES * 2);

    return ret;
}

void STGLevel::EnableBullet(Bullet *b)
{
    if (bullets_p != nullptr)
        bullets_p->Prev = b;
    b->Prev = nullptr;
    b->Next = bullets_p;
    bullets_p = b;

#ifdef _DEBUG
    std::cout << "Bullet enabled: " << b->CodeName << ".\n";
#endif
}

void STGLevel::DisableBullet(Bullet *b)
{
    if (b->Prev == nullptr)
        bullets_p = b->Next;
    else
        b->Prev->Next = b->Next;

    if (b->Next != nullptr)
        b->Next->Prev = b->Prev;

#ifdef _DEBUG
    std::cout << "Bullet disabled: " << b->CodeName << ".\n";
#endif
}

void STGLevel::HelpRespwan(int preload_id, int real_id)
{
#ifdef _DEBUG
    std::cout << "STG-" << CodeName << " respwan charactor-" << real_id
              << " with pre-load ID: " << preload_id << ".\n";
#endif

    STGCharactor &rc = onstage_charactors[records[real_id][STGCompType::SCT_CHARACTOR]];

    /* Re-Attach fixture. */
    rc.Physics->DestroyFixture(rc.Physics->GetFixtureList());
    if (standby[preload_id].MyChar.Phy.Physical)
        rc.Physics->CreateFixture(&standby[preload_id].MyChar.Phy.FD);

    /* Shooter */
    if (standby[preload_id].MyShooters != nullptr)
    {
        Shooter *sp = standby[preload_id].MyShooters;
        do
            sp = sp->Undershift(rc.Physics);
        while (sp != standby[preload_id].MyShooters);
    }

    rc.Enable(real_id, standby[preload_id].MyChar, rc.Physics,
              standby[preload_id].MyShooters, standby[preload_id].MyEnter);
}

void STGLevel::PlayerWin()
{
#ifdef _DEBUG
    std::cout << "STG-" << CodeName << " player is winning!\n";
#endif

    al_start_timer(count_down);
}

void STGLevel::PlayerDying()
{
#ifdef _DEBUG
    std::cout << "STG-" << CodeName << " player is dying!\n";
#endif

    player = nullptr;
    al_add_timer_count(count_down, 2);
    al_start_timer(count_down);
}
