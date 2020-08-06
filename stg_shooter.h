#ifndef STG_SHOOTER_H
#define STG_SHOOTER_H

#include "data_struct.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

class STGShooter
{
public:
    /* Constructor/Destructor */
    STGShooter();
    STGShooter(const STGShooter &) = delete;
    STGShooter(STGShooter &&) = delete;
    STGShooter &operator=(const STGShooter &) = delete;
    STGShooter &operator=(STGShooter &&) = delete;
    ~STGShooter() = default;

    void Undershift(const STGShooterSetting &setting);
    void Update();

    ALLEGRO_EVENT_QUEUE *Recv;

    STGShooter *Prev, *Next;

private:
    static constexpr int MAX_BULLETS = 1024u;
    static constexpr int MAX_B_TYPES = 4u;

    STGBulletSetting bullet_settings[MAX_B_TYPES];

    struct
    {
        int timer;
        b2Body *physics;
    } d_bullets[MAX_B_TYPES][MAX_BULLETS];
    b2Body *s_bullets[MAX_B_TYPES][MAX_BULLETS];
    int d_bullet_n[MAX_B_TYPES];
    int s_bullet_n[MAX_B_TYPES];
    int b_type_n;
};

#endif