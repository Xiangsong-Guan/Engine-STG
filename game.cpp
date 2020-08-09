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
    SCPattern::InitSCPattern();

    /* Make the main Lua state */
    L_top = luaL_newstate();
    luaL_openlibs(L_top);
    ResourceManager::Init(L_top);

    /* load resource */
    ResourceManager::LoadFont();
    ResourceManager::LoadSTGLevel("test_level");
    ResourceManager::LoadSTGShooter("test_shooter_slow");
    TextItem s = {u8"STG 早期测试台", ResourceManager::GetFont("source_42"),
                  .5f, .3f, ALLEGRO_ALIGN_CENTER};
    TextItem a = {u8"开始", ResourceManager::GetFont("source_36"),
                  .5f, .55f, ALLEGRO_ALIGN_CENTER};
    TextItem b = {u8"退出", ResourceManager::GetFont("source_36"),
                  .5f, .65f, ALLEGRO_ALIGN_CENTER};
    TextItem c = {u8"继续", ResourceManager::GetFont("source_36"),
                  .5f, .45f, ALLEGRO_ALIGN_CENTER};
    TextItem d = {u8"回到标题画面", ResourceManager::GetFont("source_36"),
                  .5f, .55f, ALLEGRO_ALIGN_CENTER};

    /* Load Save */
    ResourceManager::LoadSave();
    stg_level.GPlayer = ResourceManager::GetSTGChar("test_player");
    stg_level.GAmmos.insert({"test_bullet", ResourceManager::GetSTGBullet("test_bullet")});
    stg_level.GGuns.insert({"test_shooter", ResourceManager::GetSTGShooter("test_shooter")});
    stg_level.GGuns.insert({"test_shooter_slow",
                            ResourceManager::GetSTGShooter("test_shooter_slow")});

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
    s_now->Update();
    i_now->Flush();
}

void Game::ProcessInput()
{
    i_now->Update(Keys);
}

void Game::Render(float forward_time)
{
    s_now->Render(forward_time);
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
    al_unregister_event_source(title.Recv, &menu_input.InputMaster);
    title.Detach();

    stg_level.Load(Width, Height, TimeStep, ResourceManager::GetSTGLevel("test_level"), this);
    al_register_event_source(stg_level.InputConnectionTerminal(), &stg_input.InputMaster);

    i_now = &stg_input;
    s_now = &stg_level;
    State = GameState::GAME_STG;

    /* DO NOT APPLY GC IN STG RUNTIME, SLOW. */
    lua_gc(L_top, LUA_GCSTOP);
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
    /* Now GC, or memory will boooooooooooooooom! */
    lua_gc(L_top, LUA_GCRESTART);
    /* Even apply GC instantly. */
    lua_gc(L_top, LUA_GCCOLLECT);

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
}
