#ifndef INPUT_PROCESSOR_H
#define INPUT_PROCESSOR_H

#include "game_event.h"
#include "flow_controller.h"

#include <allegro5/allegro5.h>

class InputProcessor
{
protected:
    static bool state[static_cast<int>(InputAction::NUM)];

    int commands[static_cast<int>(InputAction::NUM)];
    bool filter[static_cast<int>(InputAction::NUM)];

    inline bool check_button_just_pressed(InputAction button, const bool keys[]) const noexcept;
    inline bool check_button_just_released(InputAction button, const bool keys[]) const noexcept;

    virtual void check_buttons(const bool keys[]) = 0;

public:
    GameFlowController *GameCon;
    ALLEGRO_EVENT_SOURCE InputMaster;

    InputProcessor();
    InputProcessor(const InputProcessor &) = delete;
    InputProcessor(InputProcessor &&) = delete;
    InputProcessor &operator=(const InputProcessor &) = delete;
    InputProcessor &operator=(InputProcessor &&) = delete;
    ~InputProcessor();

    void Update(const bool keys[]);
    void SetCommand(InputAction define, int key) noexcept;
    void Flush() noexcept;
};

class STGInput : public InputProcessor
{
    virtual void check_buttons(const bool keys[]) final;
};

class MenuInput : public InputProcessor
{
    virtual void check_buttons(const bool keys[]) final;
};

#endif