#ifndef INPUT_PROCESSOR_H
#define INPUT_PROCESSOR_H

#include "game_event.h"
#include "flow_controller.h"

#include <allegro5/allegro5.h>

class InputProcessor
{
protected:
    static bool state[InputAction::IA_NUM];

    int commands[InputAction::IA_NUM];
    bool filter[InputAction::IA_NUM];

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
    virtual ~InputProcessor();

    void Update(const bool keys[]);
    void SetCommand(InputAction define, int key) noexcept;
    void Flush() noexcept;
};

class STGInput : public InputProcessor
{
public:
    STGInput() = default;
    STGInput(const STGInput &) = delete;
    STGInput(STGInput &&) = delete;
    STGInput &operator=(const STGInput &) = delete;
    STGInput &operator=(STGInput &&) = delete;
    ~STGInput() = default;

private:
    void check_buttons(const bool keys[]) final;
};

class MenuInput : public InputProcessor
{
public:
    MenuInput() = default;
    MenuInput(const MenuInput &) = delete;
    MenuInput(MenuInput &&) = delete;
    MenuInput &operator=(const MenuInput &) = delete;
    MenuInput &operator=(MenuInput &&) = delete;
    ~MenuInput() = default;

private:
    void check_buttons(const bool keys[]) final;
};

#endif