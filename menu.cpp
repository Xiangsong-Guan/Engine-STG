#include "menu.h"

#include <iostream>

static inline void nothing(Menu *m) noexcept {}

/*************************************************************************************************
 *                                                                                               *
 *                                   Menu input commands                                         *
 *                                                                                               *
 *************************************************************************************************/

std::array<std::function<void(Menu *)>, InputAction::IA_NUM> Menu::commands;

void Menu::up() noexcept
{
#ifdef _DEBUG
    std::cout << "Menu cursor before: " << cursor << "\n";
#endif

    items_text[cursor].Color = al_map_rgb_f(1.f, 1.f, 1.f);
    cursor = cursor == 0 ? 0 : cursor - 1;
    /* Highlight the cursor */
    items_text[cursor].Color = al_map_rgb_f(0.f, 1.f, 0.f);

#ifdef _DEBUG
    std::cout << "Menu cursor after: " << cursor << "\n";
#endif
}

void Menu::down() noexcept
{
#ifdef _DEBUG
    std::cout << "Menu cursor before: " << cursor << "\n";
#endif

    items[cursor].TR->Color = al_map_rgb_f(1.f, 1.f, 1.f);
    cursor = cursor == btn_n - 1 ? cursor : cursor + 1;
    /* Highlight the cursor */
    items[cursor].TR->Color = al_map_rgb_f(0.f, 1.f, 0.f);

#ifdef _DEBUG
    std::cout << "Menu cursor after: " << cursor << "\n";
#endif
}

void Menu::confirm()
{
    /* Highlight the confirm */
    items_text[cursor].Color = al_map_rgb_f(1.f, 0.f, 0.f);
    items[cursor].F(this);
}

/*************************************************************************************************
 *                                                                                               *
 *                                   Menu item functions                                         *
 *                                                                                               *
 *************************************************************************************************/

std::unordered_map<std::string, std::function<void(Menu *)>> Menu::menu_cmds;

void Menu::test_start() const
{
    GameCon->LinkStart();
}

void Menu::test_end() const
{
    GameCon->LinkEnd();
}

void Menu::stg_resume() const
{
    GameCon->STGResume();
}

void Menu::stg_return() const
{
    GameCon->STGReturn(false);
}

void Menu::InitMenuStaff()
{
    commands.fill(std::function<void(Menu *)>(nothing));
    commands[InputAction::IA_MOVE_UP] = std::mem_fn(&Menu::up);
    commands[InputAction::IA_MOVE_DOWN] = std::mem_fn(&Menu::down);
    commands[InputAction::IA_ACTION] = std::mem_fn(&Menu::confirm);

    menu_cmds["start"] = std::mem_fn(&Menu::test_start);
    menu_cmds["quit"] = std::mem_fn(&Menu::test_end);
    menu_cmds["stg_resume"] = std::mem_fn(&Menu::stg_resume);
    menu_cmds["return_title"] = std::mem_fn(&Menu::stg_return);
}

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

Menu::Menu() : item_n(0), cursor(0), GameCon(nullptr)
{
    Recv = al_create_event_queue();
    if (!Recv)
    {
        std::cerr << "Failed to initialize Menu's event queue!\n";
        std::abort();
    }
}

Menu::~Menu()
{
    al_destroy_event_queue(Recv);
}

void Menu::Setup(const std::vector<TextItem> &its, const std::vector<std::string> &funcs,
                 int width, int height)
{
    item_n = 0;
    btn_n = 0;

    for (auto &&e : its)
    {
        items_text[item_n].SetText(e);

        if (btn_n < funcs.size())
        {
            items[btn_n].TR = items_text + item_n;
            items[btn_n].F = menu_cmds.at(funcs[btn_n]);

            btn_n += 1;
        }

        item_n += 1;
    }
}

void Menu::Attach() noexcept
{
    cursor = 0;
    items[cursor].TR->Color = al_map_rgb_f(0.f, 1.f, 0.f);
}

void Menu::Detach() noexcept
{
    items[cursor].TR->Color = al_map_rgb_f(1.f, 1.f, 1.f);
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void Menu::Update()
{
    ALLEGRO_EVENT event;

    while (al_get_next_event(Recv, &event))
#ifdef _DEBUG
    {
        std::cout << "Menu recv input action: " << event.user.data1 << "\n";
#endif

        commands[event.user.data1](this);

#ifdef _DEBUG
    }
#endif
}

void Menu::Render(float forward_time)
{
    for (int i = 0; i < item_n; i++)
        items_text[i].Draw();
}
