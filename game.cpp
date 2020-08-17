#include "game.h"

#include "resource_manger.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

Game::Game() : State(GameState::GAME_WELCOME), Keys() {}

void Game::Init(int width, int height, float time_step)
{
    TimeStep = time_step;
    Width = width;
    Height = height;

    /* static function init */
    Menu::InitMenuStaff();
    SpriteRenderer::InitRndrCmd();
    STGCharactor::InitInputCmd();
    STGThinker::InitSCPattern();
    STGShooter::InitSSPattern();

    /* Make the main Lua state */
    L_top = luaL_newstate();
    luaL_openlibs(L_top);
    ResourceManager::Init(L_top);

    /* load resource */
    ResourceManager::LoadFont();
    ResourceManager::LoadSTGLevel("test_level");
    TextItem s = {reinterpret_cast<const char *>(u8"STG 早期测试台"),
                  ResourceManager::GetFont("source_42"), .5f, .3f, ALLEGRO_ALIGN_CENTER};
    TextItem a = {reinterpret_cast<const char *>(u8"开始"), ResourceManager::GetFont("source_36"), .5f, .55f, ALLEGRO_ALIGN_CENTER};
    TextItem b = {reinterpret_cast<const char *>(u8"退出"), ResourceManager::GetFont("source_36"), .5f, .65f, ALLEGRO_ALIGN_CENTER};
    TextItem c = {reinterpret_cast<const char *>(u8"继续"), ResourceManager::GetFont("source_36"), .5f, .45f, ALLEGRO_ALIGN_CENTER};
    TextItem d = {reinterpret_cast<const char *>(u8"回到标题画面"), ResourceManager::GetFont("source_36"), .5f, .55f, ALLEGRO_ALIGN_CENTER};

#ifdef STG_PERFORMENCE_SHOW
    update_cost.SetText({reinterpret_cast<const char *>(u8"逻辑帧耗时："),
                         ResourceManager::GetFont("source_12"), 0.f, .95f, ALLEGRO_ALIGN_LEFT});
    render_cost.SetText({reinterpret_cast<const char *>(u8"渲染帧耗时："),
                         ResourceManager::GetFont("source_12"), .1f, .95f, ALLEGRO_ALIGN_LEFT});
    fps.SetText({reinterpret_cast<const char *>(u8"帧每秒："),
                 ResourceManager::GetFont("source_12"), .2f, .95f, ALLEGRO_ALIGN_LEFT});
    update_cost.SetWH(width, height);
    render_cost.SetWH(width, height);
    fps.SetWH(width, height);
    uc_ms.SetText({"", ResourceManager::GetFont("source_12"), update_cost.GetWidth(), .95f, ALLEGRO_ALIGN_LEFT});
    rc_ms.SetText({"", ResourceManager::GetFont("source_12"), .1f + render_cost.GetWidth(), .95f, ALLEGRO_ALIGN_LEFT});
    fps_ms.SetText({"", ResourceManager::GetFont("source_12"), .2f + fps.GetWidth(), .95f, ALLEGRO_ALIGN_LEFT});
    uc_ms.SetWH(width, height);
    rc_ms.SetWH(width, height);
    fps_ms.SetWH(width, height);
#endif

    /* Load Save */
    ResourceManager::LoadSave();
    stg_level.SPlayer = {"test_player", {"test_shooter", "test_shooter_slow"}, {"test_bullet"}};

    /* Setup title/menu screen */
    title.Setup({a, b, s}, {"start", "quit"}, width, height);
    stg_menu.Setup({c, d}, {"stg_resume", "return_title"}, width, height);

    menu_input.SetCommand(InputAction::PAUSE, ALLEGRO_KEY_ESCAPE);
    menu_input.SetCommand(InputAction::MOVE_UP, ALLEGRO_KEY_W);
    menu_input.SetCommand(InputAction::MOVE_DOWN, ALLEGRO_KEY_S);
    menu_input.SetCommand(InputAction::MOVE_LEFT, ALLEGRO_KEY_A);
    menu_input.SetCommand(InputAction::MOVE_RIGHT, ALLEGRO_KEY_D);
    menu_input.SetCommand(InputAction::ACTION, ALLEGRO_KEY_Z);
    menu_input.SetCommand(InputAction::SHIFT, ALLEGRO_KEY_LSHIFT);
    menu_input.SetCommand(InputAction::CANCEL, ALLEGRO_KEY_X);
    menu_input.SetCommand(InputAction::SPECIAL, ALLEGRO_KEY_C);
    menu_input.GameCon = this;
    /* Also setup stg input, but not link and active it untill stg start */
    stg_input.SetCommand(InputAction::PAUSE, ALLEGRO_KEY_ESCAPE);
    stg_input.SetCommand(InputAction::MOVE_UP, ALLEGRO_KEY_W);
    stg_input.SetCommand(InputAction::MOVE_DOWN, ALLEGRO_KEY_S);
    stg_input.SetCommand(InputAction::MOVE_LEFT, ALLEGRO_KEY_A);
    stg_input.SetCommand(InputAction::MOVE_RIGHT, ALLEGRO_KEY_D);
    stg_input.SetCommand(InputAction::ACTION, ALLEGRO_KEY_Z);
    stg_input.SetCommand(InputAction::SHIFT, ALLEGRO_KEY_LSHIFT);
    stg_input.SetCommand(InputAction::CANCEL, ALLEGRO_KEY_X);
    stg_input.SetCommand(InputAction::SPECIAL, ALLEGRO_KEY_C);
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
    uc_ms.ChangeText(static_cast<float>(al_get_time() - s) * 1000.f);
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
    rc_ms.ChangeText(static_cast<float>(al_get_time() - s) * 1000.f);

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

    default:
        break;
    }
}

void Game::LinkStart()
{
#ifdef _CONSOLE
    std::cout << "Lua will turn on, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif

    al_unregister_event_source(title.Recv, &menu_input.InputMaster);
    title.Detach();

    stg_level.Load(Width, Height, TimeStep, ResourceManager::GetSTGLevel("test_level"));
    al_register_event_source(stg_level.InputConnectionTerminal(), &stg_input.InputMaster);

    i_now = &stg_input;
    s_now = &stg_level;
    State = GameState::GAME_STG;

    /* DO NOT APPLY GC IN STG RUNTIME, SLOW. */
    lua_gc(L_top, LUA_GCSTOP);

#ifdef _CONSOLE
    std::cout << "Lua GC stop, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
}

void Game::LinkEnd() noexcept
{
    State = GameState::SHOULD_CLOSE;
}

void Game::STGPause()
{
    al_unregister_event_source(stg_level.InputConnectionTerminal(), &stg_input.InputMaster);

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

    al_register_event_source(stg_level.InputConnectionTerminal(), &stg_input.InputMaster);

    i_now = &stg_input;
    s_now = &stg_level;
    State = GameState::GAME_STG;
}

void Game::STGReturn(bool from_level)
{
#ifdef _CONSOLE
    std::cout << "Lua GC will turn on, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
    /* Now GC, or memory will boooooooooooooooom! */
    lua_gc(L_top, LUA_GCRESTART);

    if (from_level)
        al_unregister_event_source(stg_level.InputConnectionTerminal(), &stg_input.InputMaster);
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

#ifdef _CONSOLE
    std::cout << "Lua full GC, now with memory: " << lua_gc(L_top, LUA_GCCOUNT) << "\n";
#endif
}
