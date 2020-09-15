#include "input_processor.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

bool InputProcessor::state[InputAction::IA_NUM] = {false};

InputProcessor::InputProcessor()
{
    for (int i = 0; i < InputAction::IA_NUM; i++)
    {
        filter[i] = true;
        commands[i] = 0;
    }

    al_init_user_event_source(&InputMaster);
}

InputProcessor::~InputProcessor()
{
    al_destroy_user_event_source(&InputMaster);
}

void InputProcessor::SetCommand(InputAction define, int key) noexcept
{
    /* Keys[0] is a unregistered key, means disabled */
    if (key > -1 && key < ALLEGRO_KEY_MAX)
        commands[define] = key;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

/* make command and buffer it to event queue */
void InputProcessor::Update(const bool keys[])
{
    /* is gamer make pause or resume? */
    if (check_button_just_pressed(InputAction::IA_PAUSE, keys))
        /* just pause and no need to check buttons */
        GameCon->Esc();
    else
        /* true check happens */
        check_buttons(keys);

    /* Save state for this turn */
    for (int i = 0; i < InputAction::IA_NUM; i++)
        state[i] = keys[commands[i]];
}

void InputProcessor::Flush() noexcept
{
    for (int i = 0; i < InputAction::IA_NUM; i++)
        filter[i] = true;
}

inline bool InputProcessor::check_button_just_pressed(
    InputAction button, const bool keys[]) const noexcept
{
    if (keys[commands[button]] && !state[button])
    {
#ifdef _DEBUG
        std::cout << "Button pressed: " << al_keycode_to_name(commands[button]) << "\n";
#endif

        return true;
    }

    return false;
}

inline bool InputProcessor::check_button_just_released(
    InputAction button, const bool keys[]) const noexcept
{
    if (!keys[commands[button]] && state[button])
    {
#ifdef _DEBUG
        std::cout << "Button released: " << al_keycode_to_name(commands[button]) << "\n";
#endif

        return true;
    }

    return false;
}

/*************************************************************************************************
 *                                                                                               *
 *                            Different Type Input Processor                                     *
 *                                                                                               *
 *************************************************************************************************/

/* STG buttons check seqence */
void STGInput::check_buttons(const bool keys[])
{
    ALLEGRO_EVENT event;
    // event.type = static_cast<ALLEGRO_EVENT_TYPE>(GameEventType::INPUT_COMMAND);

    /* 4-direction move */
    if (keys[commands[InputAction::IA_MOVE_UP]] &&
        filter[InputAction::IA_MOVE_UP])
    {
        event.user.data1 = STGCharCommand::SCC_UP;
        al_emit_user_event(&InputMaster, &event, nullptr);
        filter[InputAction::IA_MOVE_UP] = false;
    }
    if (keys[commands[InputAction::IA_MOVE_DOWN]] &&
        filter[InputAction::IA_MOVE_DOWN])
    {
        event.user.data1 = STGCharCommand::SCC_DOWN;
        al_emit_user_event(&InputMaster, &event, nullptr);
        filter[InputAction::IA_MOVE_DOWN] = false;
    }
    if (keys[commands[InputAction::IA_MOVE_LEFT]] &&
        filter[InputAction::IA_MOVE_LEFT])
    {
        event.user.data1 = STGCharCommand::SCC_LEFT;
        al_emit_user_event(&InputMaster, &event, nullptr);
        filter[InputAction::IA_MOVE_LEFT] = false;
    }
    if (keys[commands[InputAction::IA_MOVE_RIGHT]] &&
        filter[InputAction::IA_MOVE_RIGHT])
    {
        event.user.data1 = STGCharCommand::SCC_RIGHT;
        al_emit_user_event(&InputMaster, &event, nullptr);
        filter[InputAction::IA_MOVE_RIGHT] = false;
    }

    /* action fire or cease */
    if (check_button_just_pressed(InputAction::IA_ACTION, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_FIRE;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
    else if (check_button_just_released(InputAction::IA_ACTION, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_CEASE;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }

    /* shift */
    if (check_button_just_pressed(InputAction::IA_SHIFT, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_CHANGE;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }

    /* skill */
    if (check_button_just_pressed(InputAction::IA_CANCEL, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_SYNC;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }

    /* bomb fire or cease */
    if (check_button_just_pressed(InputAction::IA_SPECIAL, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_FORCE_SYNC_REQUEST;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
    else if (check_button_just_released(InputAction::IA_SPECIAL, keys))
    {
        event.user.data1 = STGCharCommand::SCC_STG_FORCE_SYNC_RESPONE;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
}

/* menu input just check press down */
void MenuInput::check_buttons(const bool keys[])
{
    ALLEGRO_EVENT event;
    // event.type = static_cast<ALLEGRO_EVENT_TYPE>(GameEventType::INPUT_COMMAND);

    if (check_button_just_pressed(InputAction::IA_MOVE_UP, keys))
    {
        event.user.data1 = InputAction::IA_MOVE_UP;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
    if (check_button_just_pressed(InputAction::IA_MOVE_DOWN, keys))
    {
        event.user.data1 = InputAction::IA_MOVE_DOWN;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
    if (check_button_just_pressed(InputAction::IA_MOVE_LEFT, keys))
    {
        event.user.data1 = InputAction::IA_MOVE_LEFT;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
    if (check_button_just_pressed(InputAction::IA_MOVE_RIGHT, keys))
    {
        event.user.data1 = InputAction::IA_MOVE_RIGHT;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }

    if (check_button_just_pressed(InputAction::IA_ACTION, keys))
    {
        event.user.data1 = InputAction::IA_ACTION;
        al_emit_user_event(&InputMaster, &event, nullptr);
    }
}