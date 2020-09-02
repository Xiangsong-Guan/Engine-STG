#ifndef GAME_H
#define GAME_H

#include "input_processor.h"
#include "stg_level.h"
#include "menu.h"
#include "flow_controller.h"

#include <lua.hpp>
#include <allegro5/allegro5.h>

enum GameState
{
    GAME_WORLD,
    GAME_STG,
    GAME_WELCOME,
    STG_PAUSE,
    WORLD_PAUSE,
    SHOULD_CLOSE,

    NUM
};

/* Game holds all game-related state and functionality. */
/* Combines all game-related data into a single class for */
/* easy access to each of the components and manageability. */
class Game : public GameFlowController
{
public:
    /* Game state */
    GameState State;
    bool Keys[ALLEGRO_KEY_MAX];
    float TimeStep;

    /* Constructor/Destructor */
    Game();
    Game(const Game &) = delete;
    Game(Game &&) = delete;
    Game &operator=(const Game &) = delete;
    Game &operator=(Game &&) = delete;
    ~Game() = default;

    /* Initialize game state (load all shaders/textures/levels) */
    void Init(int width, int height, float time_step);
    void Terminate();

    /* GameLoop */
    void ProcessInput();
    void Update();
    void Render(float forward_time);

    /* command things */
    void Esc() final;
    void LinkStart() final;
    void LinkEnd() noexcept final;
    void STGPause() final;
    void STGResume() final;
    void STGReturn(bool from_level) final;

private:
    ALLEGRO_TRANSFORM T;

    STGInput stg_input;
    MenuInput menu_input;

    STGLevel stg_level;

    Menu title;
    Menu stg_menu;

#ifdef STG_PERFORMENCE_SHOW
    TextRenderer update_cost, uc_ms;
    TextRenderer render_cost, rc_ms;
    TextRenderer fps, fps_ms;
#endif

    Scene *s_now;
    InputProcessor *i_now;

    /* Share with resource manager. */
    lua_State *L_top;
};

#endif