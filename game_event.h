#ifndef GAME_EVENT_H
#define GAME_EVENT_H

#include <allegro5/allegro5.h>

enum GameEventType
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

enum Movement
{
    /*    +1
     * +3  0 -3
     *    -1    */
    MM_TO_UP = 1,
    MM_TO_DOWN = -1,
    MM_TO_LEFT = 3,
    MM_TO_RIGHT = -3,
    MM_TO_NUM = 4,

    MM_IDLE = 4,
    MM_UP = MM_IDLE + 1,
    MM_DOWN = MM_IDLE - 1,
    MM_LEFT = MM_IDLE + 3,
    MM_RIGHT = MM_IDLE - 3,
    MM_UL = MM_IDLE + 1 + 3,
    MM_UR = MM_IDLE + 1 - 3,
    MM_DL = MM_IDLE - 1 + 3,
    MM_DR = MM_IDLE - 1 - 3,
    MM_NUM = 9
};

enum InputAction
{
    IA_MOVE_UP,
    IA_MOVE_DOWN,
    IA_MOVE_LEFT,
    IA_MOVE_RIGHT,
    IA_ACTION,
    IA_SHIFT,
    IA_CANCEL,
    IA_SPECIAL,

    IA_PAUSE,

    IA_NUM
};

enum STGCharCommand
{
    SCC_UP,
    SCC_DOWN,
    SCC_LEFT,
    SCC_RIGHT,
    SCC_STG_FIRE,
    SCC_STG_CEASE,
    SCC_STG_CHANGE,
    SCC_STG_SYNC,
    SCC_STG_FORCE_SYNC_REQUEST,
    SCC_STG_FORCE_SYNC_RESPONE,

    SCC_STG_CHAIN,
    SCC_STG_UNCHAIN,

    SCC_DISABLE,
    SCC_RESPAWN,

    SCC_MOVE_XY,

    SCC_NUM
};

enum STGCharEvent
{
    SCE_GAME_OVER = 0b1,
    SCE_SUB_PATTERN_DONE = 0b10,
    SCE_OPREATIONAL = 0b100,
};

const char *const STG_CHAR_EVENT[] = {
    "GAME_OVER",
    "SUB_PATTERN_DONE",
    "OPREATIONAL",
    NULL};

const unsigned int STG_CHAR_EVENT_BIT[] = {
    STGCharEvent::SCE_GAME_OVER,
    STGCharEvent::SCE_SUB_PATTERN_DONE,
    STGCharEvent::SCE_OPREATIONAL};

enum GameRenderCommand
{
    GRC_CHANGE_TEXTURE,

    GRC_NUM
};

#endif