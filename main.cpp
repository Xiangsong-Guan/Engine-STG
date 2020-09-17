#include "game.h"
#include "cppsuckdef.h"
#include "static_init.h"

#include <allegro5/allegro5.h>

/* al static init before game setup */
StaticInit s_init;

/* the game */
Game STG;

/* the display */
ALLEGRO_DISPLAY *disp;

int main(int argc, char **argv)
{
    float prev_time_sec = 0.f, cur_time_sec = 0.f, lag = 0.f;
    ALLEGRO_EVENT_QUEUE *input_event_bus;
    ALLEGRO_EVENT event;

    input_event_bus = al_create_event_queue();
    MustInit(input_event_bus, "input event queue");

    al_set_new_display_flags(ALLEGRO_FULLSCREEN_WINDOW);
    disp = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);
    MustInit(disp, "display");
#ifdef _DEBUG
    al_set_display_flag(disp, ALLEGRO_FULLSCREEN_WINDOW, false);
    al_resize_display(disp, 1280, 720);
#endif
    /* Allegro uses pre-multi-alpha for blending, so usual blender (below) not work well,
     * make sure texture is pre-multi-alpha (which is default), below blender is 
     * used for non-pre-multi */
    // al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

    MustInit(al_install_keyboard(), "keyboard");
    al_register_event_source(input_event_bus, al_get_keyboard_event_source());

    STG.Init(al_get_display_width(disp), al_get_display_height(disp), SEC_PER_UPDATE);

    prev_time_sec = al_get_time();
    while (STG.State != GameState::SHOULD_CLOSE)
    {
        /* poll input & window event */
        while (al_get_next_event(input_event_bus, &event))
            switch (event.type)
            {
            case ALLEGRO_EVENT_KEY_DOWN:
                STG.Keys[event.keyboard.keycode] = true;
                break;

            case ALLEGRO_EVENT_KEY_UP:
                STG.Keys[event.keyboard.keycode] = false;
                break;
            }

        /* make command according to event poll, command will be buffered */
        STG.ProcessInput();

        /* time check & logic update */
        cur_time_sec = al_get_time();
        lag += cur_time_sec - prev_time_sec;
        prev_time_sec = cur_time_sec;
        while (lag >= SEC_PER_UPDATE)
        {
            STG.Update();
            lag -= SEC_PER_UPDATE;
        }

        al_clear_to_color(al_map_rgb_f(0.f, 0.f, 0.f));
        STG.Render(lag / SEC_PER_UPDATE);
        al_flip_display();
    }

    STG.Terminate();
    al_uninstall_keyboard();
    al_destroy_event_queue(input_event_bus);
    al_destroy_display(disp);
    return 0;
}