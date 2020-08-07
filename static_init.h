#ifndef ALAL_STATIC_INIT_H
#define ALAL_STATIC_INIT_H

#include <allegro5/allegro5.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>

#include <iostream>

inline void MustInit(bool test, std::string description)
{
    if (test)
        return;
    std::cerr << "Couldn't initialize " << description << std::endl;
    std::exit(1);
}

class StaticInit
{
public:
    StaticInit()
    {
        MustInit(al_init(), "allegro");
        MustInit(al_init_image_addon(), "image IO");
        MustInit(al_init_font_addon(), "font");
        MustInit(al_init_ttf_addon(), "TTF");
        MustInit(al_init_primitives_addon(), "primitives");
    }
};

#endif