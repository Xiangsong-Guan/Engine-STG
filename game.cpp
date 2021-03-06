#include "game.h"

#include "resource_manger.h"
#include "cppsuckdef.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

Game::Game() : State(GameState::GAME_WELCOME), Keys() {}

void Game::Init(int width, int height, float time_step)
{
    g_time_step = time_step;
    g_height = height;
    g_width = width;
    g_scale = static_cast<float>(height) / static_cast<float>(SCREEN_HEIGHT);
    g_offset = (static_cast<float>(g_width) - static_cast<float>(SCREEN_WIDTH) * g_scale) / 2.f;
    g_oppo_offset = static_cast<float>(width) - g_offset;

    /* static function init */
    Menu::InitMenuStaff();
    SpriteRenderer::InitRndrCmd();
    STGCharactor::InitInputCmd();
    STGThinker::InitSCPattern();
    Shooter::InitSSPattern();

    /* Make the main Lua state */
    L_top = luaL_newstate();
    luaL_openlibs(L_top);
    ResourceManager::Init(L_top);

    /* load resource */
    ResourceManager::LoadFont();
    ResourceManager::LoadSTGLevel("test_level");
    TextItem s = {reinterpret_cast<const char *>(u8"STG 早期測試"),
                  ResourceManager::GetFont("m+12b_36"),
                  .5f * static_cast<float>(g_width), .3f * static_cast<float>(g_height),
                  ALLEGRO_ALIGN_CENTER};
    TextItem a = {reinterpret_cast<const char *>(u8"開始"),
                  ResourceManager::GetFont("m+12r_24"),
                  .5f * static_cast<float>(g_width), .55f * static_cast<float>(g_height),
                  ALLEGRO_ALIGN_CENTER};
    TextItem b = {reinterpret_cast<const char *>(u8"結束"),
                  ResourceManager::GetFont("m+12r_24"),
                  .5f * static_cast<float>(g_width), .65f * static_cast<float>(g_height),
                  ALLEGRO_ALIGN_CENTER};
    TextItem c = {reinterpret_cast<const char *>(u8"継続"),
                  ResourceManager::GetFont("m+12r_24"),
                  .5f * static_cast<float>(g_width), .45f * static_cast<float>(g_height),
                  ALLEGRO_ALIGN_CENTER};
    TextItem d = {reinterpret_cast<const char *>(u8"回到標題"),
                  ResourceManager::GetFont("m+12r_24"),
                  .5f * static_cast<float>(g_width), .55f * static_cast<float>(g_height),
                  ALLEGRO_ALIGN_CENTER};

#ifdef STG_PERFORMENCE_SHOW
    al_identity_transform(&T);

    update_cost.Text = {reinterpret_cast<const char *>(u8"逻辑帧："),
                        ResourceManager::GetFont("hana_a_10"),
                        10.f, static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    uc_ms.Text = {"00.000~", ResourceManager::GetFont("hana_a_10"),
                  update_cost.GetRight(), static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    update_cost.Text.Y -= (update_cost.GetHeight() + 10.f);
    uc_ms.Text.Y -= (uc_ms.GetHeight() + 10.f);

    render_cost.Text = {reinterpret_cast<const char *>(u8"渲染帧："),
                        ResourceManager::GetFont("hana_a_10"),
                        uc_ms.GetRight(), static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    rc_ms.Text = {"00.000~", ResourceManager::GetFont("hana_a_10"),
                  render_cost.GetRight(), static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    render_cost.Text.Y -= (render_cost.GetHeight() + 10.f);
    rc_ms.Text.Y -= (rc_ms.GetHeight() + 10.f);

    fps.Text = {reinterpret_cast<const char *>(u8"帧每秒："), ResourceManager::GetFont("hana_a_10"),
                rc_ms.GetRight(), static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    fps_ms.Text = {"000", ResourceManager::GetFont("hana_a_10"),
                   fps.GetRight(), static_cast<float>(g_height), ALLEGRO_ALIGN_LEFT};
    fps.Text.Y -= (fps.GetHeight() + 10.f);
    fps_ms.Text.Y -= (fps_ms.GetHeight() + 10.f);
#endif

    /* Load Save */
    ResourceManager::LoadSave();
    stg_level.SPlayer = {"player", {"player_shooter", "player_shooter_slow"}, {"bytri"}};

    /* Setup title/menu screen */
    title.Setup({a, b, s}, {"start", "quit"}, "title");
    stg_menu.Setup({c, d}, {"stg_resume", "return_title"}, "std_menu");

    _ASSERTE(_CrtCheckMemory());

    menu_input.SetCommand(InputAction::IA_PAUSE, ALLEGRO_KEY_ESCAPE);
    menu_input.SetCommand(InputAction::IA_MOVE_UP, ALLEGRO_KEY_UP);
    menu_input.SetCommand(InputAction::IA_MOVE_DOWN, ALLEGRO_KEY_DOWN);
    menu_input.SetCommand(InputAction::IA_MOVE_LEFT, ALLEGRO_KEY_LEFT);
    menu_input.SetCommand(InputAction::IA_MOVE_RIGHT, ALLEGRO_KEY_RIGHT);
    menu_input.SetCommand(InputAction::IA_ACTION, ALLEGRO_KEY_Z);
    menu_input.SetCommand(InputAction::IA_SHIFT, ALLEGRO_KEY_LSHIFT);
    menu_input.SetCommand(InputAction::IA_CANCEL, ALLEGRO_KEY_X);
    menu_input.SetCommand(InputAction::IA_SPECIAL, ALLEGRO_KEY_C);
    menu_input.GameCon = this;
    /* Also setup stg input, but not link and active it untill stg start */
    stg_input.SetCommand(InputAction::IA_PAUSE, ALLEGRO_KEY_ESCAPE);
    stg_input.SetCommand(InputAction::IA_MOVE_UP, ALLEGRO_KEY_UP);
    stg_input.SetCommand(InputAction::IA_MOVE_DOWN, ALLEGRO_KEY_DOWN);
    stg_input.SetCommand(InputAction::IA_MOVE_LEFT, ALLEGRO_KEY_LEFT);
    stg_input.SetCommand(InputAction::IA_MOVE_RIGHT, ALLEGRO_KEY_RIGHT);
    stg_input.SetCommand(InputAction::IA_ACTION, ALLEGRO_KEY_Z);
    stg_input.SetCommand(InputAction::IA_SHIFT, ALLEGRO_KEY_LSHIFT);
    stg_input.SetCommand(InputAction::IA_CANCEL, ALLEGRO_KEY_X);
    stg_input.SetCommand(InputAction::IA_SPECIAL, ALLEGRO_KEY_C);
    stg_input.GameCon = this;

    /* Set right para for scene. */
    stg_level.GameCon = this;
    title.GameCon = this;
    stg_menu.GameCon = this;

    /* Leave all things to event handle in update function. When game leave title screen
     * just change current active scene.  */
    al_register_event_source(title.Recv, &menu_input.InputMaster);
    title.Attach();
    i_now = &menu_input;
    s_now = &title;

    _ASSERTE(_CrtCheckMemory());
}

void Game::Terminate()
{
    /* Clean All */
    lua_close(L_top);
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void Game::Update()
{
#ifdef STG_PERFORMENCE_SHOW
    double s = al_get_time();
#endif

    s_now->Update();
    i_now->Flush();

#ifdef STG_PERFORMENCE_SHOW
    uc_ms.ChangeText((al_get_time() - s) * 1000.f, 6);
#endif
}

void Game::ProcessInput()
{
    i_now->Update(Keys);
}

void Game::Render(float forward_time)
{
#ifdef STG_PERFORMENCE_SHOW
    double s = al_get_time();

    static int fps_i = 0;
    static double wait_one_sec = al_get_time();
    fps_i += 1;
    if (s - wait_one_sec > 1.)
    {
        fps_ms.ChangeText(fps_i);
        fps_i = 0;
        wait_one_sec = s;
    }
#endif

    s_now->Render(forward_time);

#ifdef STG_PERFORMENCE_SHOW
    rc_ms.ChangeText((al_get_time() - s) * 1000.f, 6);

    al_use_transform(&T);

    update_cost.Draw();
    uc_ms.Draw();
    render_cost.Draw();
    rc_ms.Draw();
    fps.Draw();
    fps_ms.Draw();
#endif
}

/*************************************************************************************************
 *                                                                                               *
 *                                   Game Flow Control                                           *
 *                                                                                               *
 *************************************************************************************************/

void Game::Esc()
{
    switch (State)
    {
    case GameState::GAME_WELCOME:
        // ...
        break;
    case GameState::STG_PAUSE:
        STGResume();
        break;
    case GameState::GAME_STG:
        STGPause();
        break;
    }
}

void Game::LinkStart()
{
#ifdef _DEBUG
    std::cout << "Lua will turn on, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif

    al_unregister_event_source(title.Recv, &menu_input.InputMaster);
    title.Detach();

    _ASSERTE(_CrtCheckMemory());

    stg_level.Load(ResourceManager::GetSTGLevel("test_level"));
    al_register_event_source(stg_level.PlayerInputTerminal, &stg_input.InputMaster);

    _ASSERTE(_CrtCheckMemory());

    i_now = &stg_input;
    s_now = &stg_level;
    State = GameState::GAME_STG;

    /* DO NOT APPLY GC IN STG RUNTIME, SLOW. */
    lua_gc(L_top, LUA_GCSTOP);

#ifdef _DEBUG
    std::cout << "Lua GC stop, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
}

void Game::LinkEnd() noexcept
{
    State = GameState::SHOULD_CLOSE;
}

void Game::STGPause()
{
    al_unregister_event_source(stg_level.PlayerInputTerminal, &stg_input.InputMaster);

    al_register_event_source(stg_menu.Recv, &menu_input.InputMaster);
    stg_menu.Attach();

    i_now = &menu_input;
    s_now = &stg_menu;
    State = GameState::STG_PAUSE;
}

void Game::STGResume()
{
    al_unregister_event_source(stg_menu.Recv, &menu_input.InputMaster);
    stg_menu.Detach();

    al_register_event_source(stg_level.PlayerInputTerminal, &stg_input.InputMaster);

    i_now = &stg_input;
    s_now = &stg_level;
    State = GameState::GAME_STG;
}

void Game::STGReturn(bool from_level)
{
#ifdef _DEBUG
    std::cout << "Lua GC will turn on, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
    /* Now GC, or memory will boooooooooooooooom! */
    lua_gc(L_top, LUA_GCRESTART);

    if (from_level)
        al_unregister_event_source(stg_level.PlayerInputTerminal, &stg_input.InputMaster);
    else
    {
        al_unregister_event_source(stg_menu.Recv, &menu_input.InputMaster);
        stg_menu.Detach();
    }

    al_register_event_source(title.Recv, &menu_input.InputMaster);
    title.Attach();

    stg_level.Unload();

    i_now = &menu_input;
    s_now = &title;
    State = GameState::GAME_WELCOME;

    /* Even apply GC instantly. */
    lua_gc(L_top, LUA_GCCOLLECT);

#ifdef _DEBUG
    std::cout << "Lua full GC, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
}
