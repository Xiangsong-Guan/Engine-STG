#ifndef STG_MUTI_RENDERER
#define STG_MUTI_RENDERER

#include "cppsuckdef.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

class BulletsRenderer
{
private:
    float forward_time;

    float xscale;
    float yscale;
    float physcale;

public:
    /* Constructor/Destructor */
    BulletsRenderer() = default;
    BulletsRenderer(const BulletsRenderer &) = delete;
    BulletsRenderer(BulletsRenderer &&) = delete;
    BulletsRenderer &operator=(const BulletsRenderer &);
    BulletsRenderer &operator=(BulletsRenderer &&) = delete;
    ~BulletsRenderer() = default;

    ALLEGRO_COLOR Color;

    inline void SetScale(float x, float y, float phy) noexcept
    {
        yscale = y;
        xscale = x;
        physcale = phy;
    }

    inline void Reset(float ft)
    {
        Color = al_map_rgb_f(1.f, 1.f, 1.f);
        forward_time = ft;
        al_hold_bitmap_drawing(true);
    }

    inline void Draw(const b2Body *b, ALLEGRO_BITMAP *s_sprite)
    {
        b2Vec2 position = b->GetPosition();
        b2Vec2 velocity = b->GetLinearVelocity();
        float rotate = b->GetAngle();

        /* update render status then */
        position += forward_time * SEC_PER_UPDATE * velocity;
        position *= physcale;

        /* begin to draw */
        al_draw_tinted_scaled_rotated_bitmap(s_sprite, Color,
                                             static_cast<float>(al_get_bitmap_width(s_sprite)) / 2.f,
                                             static_cast<float>(al_get_bitmap_height(s_sprite)) / 2.f,
                                             position.x, position.y, xscale, yscale, rotate, 0);
    }

    inline void Flush()
    {
        al_hold_bitmap_drawing(false);
    }
};

#endif