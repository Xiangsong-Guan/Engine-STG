#ifndef STG_MUTI_RENDERER
#define STG_MUTI_RENDERER

#include "cppsuckdef.h"
#include "data_struct.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

class BulletsRenderer
{
private:
    float forward_time;

public:
    /* Constructor/Destructor */
    BulletsRenderer() = default;
    BulletsRenderer(const BulletsRenderer &) = delete;
    BulletsRenderer(BulletsRenderer &&) = delete;
    BulletsRenderer &operator=(const BulletsRenderer &);
    BulletsRenderer &operator=(BulletsRenderer &&) = delete;
    ~BulletsRenderer() = default;

    ALLEGRO_COLOR Color;

    inline void Reset(float ft)
    {
        Color = al_map_rgb_f(1.f, 1.f, 1.f);
        forward_time = ft;
        al_hold_bitmap_drawing(true);
    }

    inline void Draw(const b2Body *b, const SpriteItem *s_sprite)
    {
        b2Vec2 position = b->GetPosition();
        b2Vec2 velocity = b->GetLinearVelocity();
        float rotate = b->GetAngle();

        /* update render status then */
        position += forward_time * SEC_PER_UPDATE * velocity;
        position *= PIXIL_PRE_M;

        /* begin to draw */
        al_draw_tinted_rotated_bitmap(s_sprite->Sprite, Color, s_sprite->CX, s_sprite->CY, position.x, position.y, rotate, 0);
    }

    inline void Flush()
    {
        al_hold_bitmap_drawing(false);
    }
};

#endif