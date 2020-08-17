#ifndef STG_SHOOTER_H
#define STG_SHOOTER_H

#include "data_struct.h"
#include "flow_controller.h"

#ifdef STG_DEBUG_PHY_DRAW
#include "physical_draw.h"
#endif

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <string>
#include <queue>
#include <functional>

struct Bullet
{
    float Speed;
    int Damage;
    KinematicSeq KS;
    PhysicalFixture Phy;
};

struct DynamicBullet
{
    int Timer;
    int KSI;
    int Type;
    b2Body *Physics;
};

class STGShooter
{
public:
    static void InitSSPattern();

    int ID;
    std::string Name;

    /* Constructor/Destructor */
    STGShooter();
    STGShooter(const STGShooter &) = delete;
    STGShooter(STGShooter &&) = delete;
    STGShooter &operator=(const STGShooter &);
    STGShooter &operator=(STGShooter &&) = delete;
    ~STGShooter() = default;

    void Load(const float b[4], const STGShooterSetting &setting, std::queue<STGBulletSetting> &bss);
    STGShooter *Undershift(int id, const b2Body *body, b2World *w, ALLEGRO_EVENT_SOURCE *rm, const b2Body *tg) noexcept;
    void MyDearPlayer() noexcept;
    STGShooter *Update();

    void Fire() noexcept;
    void Cease() noexcept;
    bool ShiftOut() noexcept;
    float ShiftIn(bool firing) noexcept;
    void Sync();
    void FSync();
    void Destroy();

    STGShooter *Prev, *Next;
    STGShooter *Shift;

    STGFlowController *Con;

#ifdef STG_DEBUG_PHY_DRAW
    PhysicalDraw *DebugDraw;
    inline STGShooter *DrawDebugData()
    {
        b2Color yellow = b2Color(1.f, 1.f, 0.f);
        if (code == SSPatternsCode::TRACK)
            DebugDraw->DrawSegment(physical->GetPosition() + my_xf.p,
                                   physical->GetPosition() + my_xf.p + (50.f * lunchers_clc_dir[0]), yellow);
        for (int s = 0; s < luncher_n; s++)
            /* Charactor will never rot, use pos instead of GetWorldPoint will be more efficiency. */
            DebugDraw->DrawSolidCircle(physical->GetPosition() + lunchers_clc_pos[s], .2f, lunchers_clc_dir[s], yellow);
        return Next;
    }
#endif

private:
    static constexpr int MAX_D_BULLETS = 256;
    static constexpr int MAX_S_BULLETS = 512;
    static constexpr int MAX_B_TYPES = 4;
    static constexpr int MAX_LUNCHER_NUM = 8;

    float bound[4];

    int rate;
    int power;
    float speed;

    bool firing;
    bool formation;

    const b2Body *physical;
    const b2Body *target;
    b2World *world;
    ALLEGRO_EVENT_SOURCE *render_master;

    /* Update per movement (only used to update lunchers' attitude)
     * local rot in charactor's local coodinate, not in world. */
    b2Transform my_xf;
    float my_angle;
    /* Cache the lunchers' charactor-local-coodinate attitude, save some calculation for fire. */
    b2Vec2 lunchers_clc_dir[MAX_LUNCHER_NUM];
    b2Vec2 lunchers_clc_pos[MAX_LUNCHER_NUM];
    float lunchers_clc_angle[MAX_LUNCHER_NUM];

    int timer;
    Luncher lunchers[MAX_LUNCHER_NUM];
    int luncher_n;

    Bullet bullets[MAX_B_TYPES];
    int ammo_slot_n;
    b2BodyDef bd;

    DynamicBullet d_bullets[MAX_D_BULLETS];
    b2Body *s_bullets[MAX_S_BULLETS];
    int d_bullet_n;
    int s_bullet_n;

    SSPatternsCode code;
    SSPatternData data;
    std::function<void(STGShooter *)> pattern;

    /* Lua AI can set sub-pattern, also use data. */
    lua_State *AI;
    SSPatternsCode sub_ptn;

    static std::function<void(STGShooter *)> patterns[static_cast<int>(SSPatternsCode::NUM)];
    void controlled();
    void stay();
    void total_turn();
    void split_turn();
    void track_enemy();
    void track_player();

    inline void init();
    inline void update_phy(int s);
    inline void luncher_track() noexcept;
    inline void update_my_attitude() noexcept;
    inline void update_lunchers_attitude() noexcept;
    inline void recalc_lunchers_attitude_cache(int s) noexcept;
    inline void fire(int s);
};

#endif