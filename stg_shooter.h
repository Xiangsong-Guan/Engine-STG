#ifndef STG_SHOOTER_H
#define STG_SHOOTER_H

#include "data_struct.h"
#include "flow_controller.h"

#ifdef STG_DEBUG_PHY_DRAW
#include "physical_draw.h"
#endif

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
    STGShooter *Undershift(int id, const b2Body *body, b2World *w, ALLEGRO_EVENT_SOURCE *rm) noexcept;
    STGShooter *Update();

    void Fire() noexcept;
    void Cease() noexcept;
    void ShiftOut() noexcept;
    float ShiftIn() noexcept;
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
        for (int s = 0; s < luncher_n; s++)
            DebugDraw->DrawSolidCircle(physical->GetPosition() + my_atti.Pos + lunchers[s].DAttitude.Pos, .2f, b2Vec2(0.f, -1.f), yellow);
        return Next;
    }
#endif

private:
    static constexpr int MAX_BULLETS = 512;
    static constexpr int MAX_B_TYPES = 4;
    static constexpr int MAX_LUNCHER_NUM = 8;

    float bound[4];

    int rate;
    int power;
    float speed;

    bool firing;
    bool formation;

    const b2Body *physical;
    b2World *world;
    ALLEGRO_EVENT_SOURCE *render_master;

    /* Update per movement */
    b2Transform my_xf;
    Attitude my_atti;

    /* Update per movement */
    long long timer;
    Luncher lunchers[MAX_LUNCHER_NUM];
    int luncher_n;

    Bullet bullets[MAX_B_TYPES];
    int ammo_slot_n;
    b2BodyDef bd;

    DynamicBullet d_bullets[MAX_BULLETS];
    b2Body *s_bullets[MAX_BULLETS];
    int d_bullet_n;
    int s_bullet_n;

    SSPatternsCode code;
    SSPatternData data;
    std::function<void(STGShooter *)> pattern;

    static std::function<void(STGShooter *)> patterns[static_cast<int>(SSPatternsCode::NUM)];
    void controlled();
    void stay();
    void total_turn();

    inline void update_phy(int s);
    inline void update_my_attitude() noexcept;
    inline void update_luncher_attitude(int s) noexcept;
    inline void update_attitude(Attitude &a, const KinematicPhase &k) noexcept;
    inline void fire(int s);
};

#endif