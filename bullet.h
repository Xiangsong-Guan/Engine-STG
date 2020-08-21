#ifndef STG_BULLET_H
#define STG_BULLET_H

#include "anime.h"
#include "data_struct.h"
#include "flow_controller.h"
#include "bullets_renderer.h"

#include <box2d/box2d.h>

#include <functional>

struct StaticBullet
{
    int ATimer;
    b2Body *Physics;
    int MasterPower;
};

struct DynamicBullet
{
    int KTimer;
    int KSI;
    StaticBullet SB;
};

class Bullet
{
private:
    static constexpr int MAX_D_BULLETS = 512;
    static constexpr int MAX_S_BULLETS = 1024;
    static constexpr int MAX_JUST_BULLETS = 128;
    static constexpr int MAX_DEAD_BULLETS = 128;

    float bound[4];
    b2World *world;
    b2BodyDef bd;

    const b2Body *target;

    int damage;
    KinematicSeq ks;
    PhysicalFixture phy;

    BulletsRenderer draw;
    Anime born;
    Anime idle;
    Anime disable;

    StaticBullet just_bullets[MAX_JUST_BULLETS];
    int just_bullets_n;
    DynamicBullet fly_bullets[MAX_D_BULLETS];
    int fly_bullets_n;
    StaticBullet flt_bullets[MAX_S_BULLETS];
    int flt_bullets_n;
    StaticBullet dead_bullets[MAX_DEAD_BULLETS];
    int dead_bullets_n;

    bool lost;
    bool need_adjust_front_for_flt;

    inline void kinematic_update(b2Body *b, const KinematicPhase &kp);
    inline void track_update_dd(b2Body *s, const b2Body *d);
    inline void track_update_nd(b2Body *s, const b2Body *d);
    inline void adjust_front(b2Body *b);
    inline void check_flt_dd();
    inline void check_flt_nd();

    void track_player();
    void track_enemy();
    void kinematic();
    void statical();
    void track_player_d();
    void track_enemy_d();
    void kinematic_d();
    void statical_d();
    std::function<void(Bullet *)> pattern;

public:
    STGFlowController *Con;
    Bullet *Prev, *Next;

    static void InitBulletPatterns();

    /* Constructor/Destructor */
    Bullet();
    Bullet(const Bullet &) = delete;
    Bullet(Bullet &&) = delete;
    Bullet &operator=(const Bullet &);
    Bullet &operator=(Bullet &&) = delete;
    ~Bullet() = default;

    void Load(const STGBulletSetting &bs, const b2Filter &f, b2World *w);
    void SetScale(float x, float y, float phy, const float b[4]) noexcept;
    Bullet *Update();
    Bullet *Draw(float forward_time);
    void Bang(const b2Vec2 &world_pos, float world_angle, int mp);

#ifdef STG_PERFORMENCE_SHOW
    inline int GetBulletNum() const
    {
        return just_bullets_n + fly_bullets_n + flt_bullets_n + dead_bullets_n;
    }
#endif
};

#endif