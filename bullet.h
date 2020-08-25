#ifndef STG_BULLET_H
#define STG_BULLET_H

#include "anime.h"
#include "data_struct.h"
#include "flow_controller.h"
#include "bullets_renderer.h"
#include "contact_listener.h"

#include <box2d/box2d.h>

#include <functional>
#include <string>

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

class Bullet;
class BulletCollisionHandler;
inline void cpp_class_sucks(Bullet *bullet, const BulletCollisionHandler *who);
inline void cpp_class_sucks_forever(Bullet *bullet, CollisionHandler *who);

class BulletCollisionHandler : public CollisionHandler
{
public:
    BulletCollisionHandler() = default;
    BulletCollisionHandler(const BulletCollisionHandler &) = delete;
    BulletCollisionHandler(BulletCollisionHandler &&) = delete;
    BulletCollisionHandler &operator=(const BulletCollisionHandler &);
    BulletCollisionHandler &operator=(BulletCollisionHandler &&) = delete;
    ~BulletCollisionHandler() = default;

    int PhysicsIndex;
    bool IsStatical;
    Bullet *Master;

    void Hit(CollisionHandler *o) final
    {
        cpp_class_sucks_forever(Master, o);
    }

    void Hurt(const STGChange *c) final
    {
        cpp_class_sucks(Master, this);
    }
};

class Bullet
{
private:
    static constexpr int MAX_BULLETS = 1024;
    static constexpr int MAX_JUST_BULLETS = 512;
    static constexpr int MAX_DEAD_BULLETS = 128;

    float bound[4];
    b2World *world;
    b2BodyDef bd;

    const b2Body *target;

    KinematicSeq ks;
    PhysicalFixture phy;

    BulletsRenderer draw;
    Anime born;
    Anime idle;
    Anime hit;

    StaticBullet just_bullets[MAX_JUST_BULLETS];
    int just_bullets_n;
    DynamicBullet fly_bullets[MAX_BULLETS];
    int fly_bullets_n;
    StaticBullet flt_bullets[MAX_BULLETS];
    int flt_bullets_n;
    StaticBullet dead_bullets[MAX_DEAD_BULLETS];
    int dead_bullets_n;

    bool lost;
    bool need_adjust_front_for_flt;

    /* pool */
    BulletCollisionHandler collision_proxy[MAX_BULLETS];
    bool used_proxy[MAX_BULLETS];
    int proxy_hint;
    inline int get_proxy() noexcept;
    inline void return_proxy(int id) noexcept;
    inline void reset_proxy() noexcept;

    inline void outsider_flt(int s);
    inline void outsider_fly(int s);
    inline void make_cp_just(int i, bool is_statical, int n);
    inline void move_from_fly_to_flt_with_cp(int s);
    inline void move_from_just_to_flt(int i);
    inline void move_from_just_to_fly(int i);
    inline void track_a_part();
    inline void kinematic_a_part();
    inline void statical_a_part();

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
    std::string Name;
    std::string CodeName;

    STGFlowController *Con;
    Bullet *Prev, *Next;

    STGChange Change;

    void Disappear(const BulletCollisionHandler *who);

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