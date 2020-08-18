#ifndef STG_BULLET_H
#define STG_BULLET_H

#include "anime.h"
#include "data_struct.h"
#include "flow_controller.h"
// #include "muti_renderer.h"

#include <box2d/box2d.h>

#include <functional>

enum class STGBulletPatternCode
{
    TRACK_PLAYER,
    TRACK_ENEMY,
    KINEMATIC,
    STATIC,

    NUM
};

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
    static constexpr int MAX_BULLETS = 512;
    static constexpr int MAX_JUST_BULLETS = 64;
    static constexpr int MAX_DEAD_BULLETS = 32;

    float bound[4];
    b2World *world;
    b2BodyDef bd;

    const b2Body *target;

    int damage;
    KinematicSeq ks;
    PhysicalFixture phy;

    // MutiRenderer draw;
    Anime born;
    Anime idle;
    Anime disable;

    StaticBullet just_bullets[MAX_JUST_BULLETS];
    int just_bullets_n;
    DynamicBullet fly_bullets[MAX_BULLETS];
    int fly_bullets_n;
    StaticBullet dead_bullets[MAX_DEAD_BULLETS];
    int dead_bullets_n;

    bool lost;

    inline void kinematic_update(const DynamicBullet &db, const KinematicPhase &kp);
    inline void track_update(b2Body *s, const b2Body *d);
    inline void check_outside();

    static std::function<void(Bullet *)> patterns[static_cast<int>(STGBulletPatternCode::NUM)];
    void track_player();
    void track_enemy();
    void kinematic();
    void statical();
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
};

#endif