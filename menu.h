#ifndef GAME_MENU_H
#define GAME_MENU_H

#include "scene.h"
#include "text_renderer.h"
#include "game_event.h"
#include "flow_controller.h"
#include "data_struct.h"

#include <allegro5/allegro5.h>

#include <string>
#include <array>
#include <vector>
#include <functional>
#include <unordered_map>

class Menu;

struct MenuBtn
{
    std::function<void(Menu *)> F;
    TextRenderer *TR;
};

class Menu : public Scene
{
public:
    static void InitMenuStaff();

    ALLEGRO_EVENT_QUEUE *Recv;

    GameFlowController *GameCon;

    Menu();
    Menu(const Menu &) = delete;
    Menu(Menu &&) = delete;
    Menu &operator=(const Menu &) = delete;
    Menu &operator=(Menu &&) = delete;
    ~Menu();

    void Setup(const std::vector<TextItem> &items, const std::vector<std::string> &funcs,
               int width, int height);
    void Attach() noexcept;
    void Detach() noexcept;

    void Update() final;
    void Render(float forward_time) final;

private:
    static constexpr int MAX_MENU_ITEM = 32;

    MenuBtn items[MAX_MENU_ITEM];
    TextRenderer items_text[MAX_MENU_ITEM];
    int item_n, btn_n;
    int cursor;

    /* Menu input commands */
    static std::array<std::function<void(Menu *)>, InputAction::IA_NUM> commands;
    void up() noexcept;
    void down() noexcept;
    void confirm();

    /* Menu item cmd store. */
    static std::unordered_map<std::string, std::function<void(Menu *)>> menu_cmds;
    void test_start() const;
    void test_end() const;
    void stg_resume() const;
    void stg_return() const;
};

#endif