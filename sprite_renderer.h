#ifndef SPRITE_RENDERER_H
#define SPRITE_RENDERER_H

#include "game_event.h"
#include "data_struct.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <functional>
#include <array>

class SpriteRenderer
{
public:
    int ID;

    /* somr render things */
    SpriteItem Sprite;
    ALLEGRO_COLOR Color;

    /* Connection with outside */
    ALLEGRO_EVENT_QUEUE *Recv;

    SpriteRenderer();
    SpriteRenderer(const SpriteRenderer &) = delete;
    SpriteRenderer(SpriteRenderer &&) = delete;
    SpriteRenderer &operator=(const SpriteRenderer &) = delete;
    SpriteRenderer &operator=(SpriteRenderer &&) = delete;
    ~SpriteRenderer();
    void CPPSuckSwap(SpriteRenderer &) noexcept;

    /* Reusable, do not have init/disable function. */
    void Show(int id, b2Body *body, const SpriteItem &first) noexcept;
    void Draw(float forward_time);

    /* Init commands list. */
    static void InitRndrCmd();

private:
    const b2Body *physics;
    float cx, cy;

    /* Command Things */
    static std::array<std::function<void(SpriteRenderer *, const ALLEGRO_EVENT *)>,
                      GameRenderCommand::GRC_NUM>
        commands;
    void change_texture(const ALLEGRO_EVENT *e) noexcept;
};

#endif