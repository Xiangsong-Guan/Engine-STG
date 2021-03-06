#include "sprite_renderer.h"

#include "cppsuckdef.h"

#include <iostream>

static inline void nothing(SpriteRenderer *r, const ALLEGRO_EVENT *e) noexcept {}

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

SpriteRenderer::SpriteRenderer()
{
    Recv = al_create_event_queue();
    if (!Recv)
    {
        std::cerr << "Failed to initialize STG charactors' input terminal\n";
        std::abort();
    }
}

SpriteRenderer::~SpriteRenderer()
{
    al_destroy_event_queue(Recv);
}

void SpriteRenderer::CPPSuckSwap(SpriteRenderer &o) noexcept
{
#ifdef _DEBUG
    std::cout << "Renderer-" << o.ID << " is moving, while " << ID << " is flawed.\n";
#endif

    this->ID = o.ID;
    this->Sprite = o.Sprite;
    this->Color = o.Color;
    this->physics = o.physics;
    std::swap(this->Recv, o.Recv);
}

void SpriteRenderer::Show(int id, b2Body *body, const SpriteItem &first) noexcept
{
    ID = id;
    physics = body;
    Sprite = first;
    Color = al_map_rgb_f(1.f, 1.f, 1.f);
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void SpriteRenderer::Draw(float forward_time)
{
    ALLEGRO_EVENT event;

    b2Vec2 position = physics->GetPosition();
    b2Vec2 velocity = physics->GetLinearVelocity();
    float rotate = physics->GetAngle();

    /* update render status then */
    position += forward_time * SEC_PER_UPDATE * velocity;
    position *= PIXIL_PRE_M;

    /* make change */
    while (al_get_next_event(Recv, &event))
        commands[event.user.data1](this, &event);

    /* begin to draw */
    al_draw_tinted_rotated_bitmap(Sprite.Sprite, Color, Sprite.CX, Sprite.CY,
                                  position.x, position.y, rotate, 0);
}

/*************************************************************************************************
 *                                                                                               *
 *                                     Render Commands                                           *
 *                                                                                               *
 *************************************************************************************************/

std::array<std::function<void(SpriteRenderer *, const ALLEGRO_EVENT *)>,
           GameRenderCommand::GRC_NUM>
    SpriteRenderer::commands;

void SpriteRenderer::InitRndrCmd()
{
    commands.fill(std::function<void(SpriteRenderer *, const ALLEGRO_EVENT *)>(nothing));
    commands[GameRenderCommand::GRC_CHANGE_TEXTURE] = std::mem_fn(&SpriteRenderer::change_texture);
}

void SpriteRenderer::change_texture(const ALLEGRO_EVENT *e) noexcept
{
    Sprite = *reinterpret_cast<const SpriteItem *>(e->user.data2);
}
