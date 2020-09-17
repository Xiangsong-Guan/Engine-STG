#ifndef STG_SHOOTER_H
#define STG_SHOOTER_H

#include "data_struct.h"
#include "flow_controller.h"
#include "bullet.h"

#ifdef STG_DEBUG_PHY_DRAW
#include "physical_draw.h"
#endif

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <string>
#include <queue>
#include <functional>

class Shooter
{
public:
    static void InitSSPattern();

    std::string Name;
    std::string CodeName;

    /* Constructor/Destructor */
    Shooter() = default;
    Shooter(const Shooter &) = delete;
    Shooter(Shooter &&) = delete;
    Shooter &operator=(const Shooter &);
    Shooter &operator=(Shooter &&) = delete;
    ~Shooter() = default;

    void Load(const float b[4], const STGShooterSetting &setting, std::queue<Bullet *> &bss);
    Shooter *Undershift(const b2Body *body) noexcept;
    void MyDearPlayer() noexcept;
    Shooter *Update();

    bool IsFiring() const noexcept;
    void SetFire(bool f) noexcept;
    Shooter *ShiftOut(bool need_ret = true) noexcept;
    float ShiftIn() noexcept;
    void LinkChain(int n) noexcept;
    int Breakchain() noexcept;

    bool IsConnected() const noexcept;
    void Heal(int curing) noexcept;
    bool Hurt(int damage) noexcept;

    Shooter *Prev, *Next;
    Shooter *ShiftDown, *ShiftUp;
    Shooter *Chain;

    STGFlowController *Con;

#ifdef STG_DEBUG_PHY_DRAW
    PhysicalDraw *DebugDraw;
    inline Shooter *DrawDebugData()
    {
        const b2Color yellow = b2Color(1.f, 1.f, 0.f);
        const b2Vec2 front = b2Vec2(0.f, -1.f);
        if (code == SSPatternsCode::SSPC_TRACK)
            DebugDraw->DrawSegment(physical->GetPosition() + my_xf.p,
                                   physical->GetPosition() + 30.f * b2Mul(my_xf, front), yellow);
        for (int s = 0; s < luncher_n; s++)
            /* Charactor will never rot, 
             * use pos instead of GetWorldPoint will be more efficiency. */
            DebugDraw->DrawSolidCircle(physical->GetPosition() + lunchers_clc_pos[s],
                                       .2f, b2Mul(my_xf, front), yellow);
        return Next;
    }
#endif

private:
    static constexpr int MAX_B_TYPES = 4;

    int rate;
    int power;
    float speed;

    bool firing;

    const b2Body *physical;
    const b2Body *target;

    /* Update per movement (only used to update lunchers' attitude)
     * local rot in charactor's local coodinate, not in world. */
    b2Transform my_xf;
    float my_angle;
    /* Cache the lunchers' charactor-local-coodinate attitude, save some calculation for fire. */
    b2Vec2 lunchers_clc_pos[MAX_LUNCHERS_NUM];
    float lunchers_clc_angle[MAX_LUNCHERS_NUM];

    int timer;
    Luncher lunchers[MAX_LUNCHERS_NUM];
    int luncher_n;

    Bullet *bullets[MAX_B_TYPES];
    int ammo_slot_n;

    SSPatternsCode code;
    SSPatternData data;
    std::function<void(Shooter *)> pattern;

    /* Lua AI can set sub-pattern, also use data. */
    lua_State *AI;
    SSPatternsCode sub_ptn;

    static std::function<void(Shooter *)> patterns[SSPatternsCode::SSPC_NUM];
    void controlled();
    void stay();
    void total_turn();
    void split_turn();
    void track_enemy();
    void track_player();

    inline void init();
    inline void track() noexcept;
    inline void recalc_lunchers_attitude_cache(int s) noexcept;
    inline void fire(int s);
};

#endif