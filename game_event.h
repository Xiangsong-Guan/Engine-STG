#ifndef GAME_EVENT_H
#define GAME_EVENT_H

#include <allegro5/allegro5.h>

enum class GameEventType
{
    // NULL
    NO_MORE = ALLEGRO_GET_EVENT_TYPE('S', 'T', 'G', 'E'),

    INPUT_COMMAND,
    RENDER_COMMAND,
    // GAME_STATE_CHANGE,
};

// enum class GameStateChange
// {
//     LINK_START,
//     LINK_END,
//     STG_PAUSE,
//     STG_RESUME,
//     STG_RETURN,

//     NUM
// };

enum class Movement
{
    /*    +1
     * +3  0 -3
     *    -1    */
    TO_UP = 1,
    TO_DOWN = -1,
    TO_LEFT = 3,
    TO_RIGHT = -3,
    TO_NUM = 4,

    IDLE = 4,
    UP = IDLE + 1,
    DOWN = IDLE - 1,
    LEFT = IDLE + 3,
    RIGHT = IDLE - 3,
    UL = IDLE + 1 + 3,
    UR = IDLE + 1 - 3,
    DL = IDLE - 1 + 3,
    DR = IDLE - 1 - 3,
    NUM = 9,

    B_I = 0b0000,
    B_U = 0b1000,
    B_D = 0b0100,
    B_L = 0b0010,
    B_R = 0b0001
};

enum class InputAction
{
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    ACTION,
    SHIFT,
    CANCEL,
    SPECIAL,

    PAUSE,

    NUM,
};

enum class STGCharCommand
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
    STG_FIRE,
    STG_CEASE,
    STG_CHANGE,
    STG_SYNC,
    STG_FORCE_SYNC_REQUEST,
    STG_FORCE_SYNC_RESPONE,

    DISABLE,
    RESPAWN,

    MOVE_XY,

    NUM
};

enum class STGCharEvent
{
    GAME_OVER,
    HP_DEC,

    NUM
};

enum class GameRenderCommand
{
    CHANGE_TEXTURE,

    NUM
};

#endif