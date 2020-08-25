#include "shooter.h"

#include "resource_manger.h"
#include "game_event.h"

#include <lua.hpp>

#include <iostream>

inline void Shooter::init()
{
    my_xf = b2Transform(b2Vec2_zero, b2Rot(0.f));
    my_angle = 0.f;

    timer = -1;
    firing = false;
    rate = 100;

    sub_ptn = SSPatternsCode::CONTROLLED;
    if (code == SSPatternsCode::CONTROLLED)
    {
        int good, rn;

        AI = ResourceManager::GetCoroutine(ResourceManager::STG_SHOOT_FUNCTIONS_KEY, CodeName);
        good = lua_resume(AI, nullptr, 0, &rn);

#ifdef STG_LUA_API_ARG_CHECK
        if (good == LUA_OK)
            std::cerr << "STG shooter" << CodeName << " lua error: return in init call!" << std::endl;
        else if (good != LUA_YIELD)
            std::cerr << "STG shooterr" << CodeName << " lua error: " << lua_tostring(AI, -1) << std::endl;
#endif
    }
}

Shooter &Shooter::operator=(const Shooter &o)
{
    Name = o.Name;
    CodeName = o.CodeName;
    power = o.power;
    speed = o.speed;
    ammo_slot_n = o.ammo_slot_n;
    luncher_n = o.luncher_n;

    std::memcpy(lunchers_clc_angle, o.lunchers_clc_angle, sizeof(float) * luncher_n);
    std::memcpy(lunchers_clc_pos, o.lunchers_clc_pos, sizeof(b2Vec2) * luncher_n);
    std::memcpy(lunchers, o.lunchers, sizeof(Luncher) * luncher_n);
    std::memcpy(bullets, o.bullets, sizeof(Bullet *) * ammo_slot_n);

    code = o.code;
    pattern = patterns[static_cast<int>(code)];
    data = o.data;

    init();

    return *this;
}

void Shooter::Load(const float b[4], const STGShooterSetting &setting, std::queue<Bullet *> &bss)
{
    Name = setting.Name;
    CodeName = setting.CodeName;
    power = setting.Power;
    speed = setting.Speed;
    ammo_slot_n = setting.AmmoSlotsNum;
    luncher_n = setting.LuncherSize;

    assert(ammo_slot_n <= MAX_B_TYPES);
    assert(luncher_n <= MAX_LUNCHERS_NUM);

    const b2Vec2 front = b2Vec2(0.f, -1.f);
    for (int s = 0; s < luncher_n; s++)
    {
        lunchers[s] = setting.Lunchers[s];
        lunchers_clc_angle[s] = lunchers[s].DAttitude.Angle;
        lunchers_clc_pos[s] = lunchers[s].DAttitude.Pos;
    }

    for (int s = 0; s < ammo_slot_n; s++)
    {
        bullets[s] = bss.front();
        if (bss.size() > 1)
            bss.pop();
    }

    code = setting.Pattern;
    pattern = patterns[static_cast<int>(code)];
    data = setting.Data;

    init();
}

Shooter *Shooter::Undershift(const b2Body *body, ALLEGRO_EVENT_SOURCE *rm) noexcept
{
    physical = body;
    render_master = rm;

    return Shift;
}

/* Player are special, need get target from world. */
void Shooter::MyDearPlayer() noexcept
{
    if (code == SSPatternsCode::TRACK)
        pattern = std::mem_fn(&Shooter::track_enemy);
}

Shooter *Shooter::Update()
{
    pattern(this);
    return Next;
}

inline void Shooter::track() noexcept
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    /* Charactor will never rot, use pos instead of GetWorldPoint will be more efficiency. */
    b2Vec2 world_diff = target->GetPosition() - (physical->GetPosition() + my_xf.p);
    world_diff.Normalize();

    my_xf.q.c = b2Dot(world_diff, front);
    my_xf.q.s = -b2Cross(world_diff, front);

    my_angle = my_xf.q.GetAngle();
}

/* Using transform mul, low priority. */
inline void Shooter::recalc_lunchers_attitude_cache(int s) noexcept
{
    lunchers_clc_angle[s] = my_angle + lunchers[s].DAttitude.Angle;
    lunchers_clc_pos[s] = b2Mul(my_xf, lunchers[s].DAttitude.Pos);
}

inline void Shooter::fire(int s)
{
    /* Charactor will never rot, use pos instead of GetWorldPoint will be more efficiency.
     * Charactor will never rot, no need to plus charactor's angle. */
    bullets[lunchers[s].AmmoSlot]->Bang(physical->GetPosition() + lunchers_clc_pos[s], lunchers_clc_angle[s], power);
}

/*************************************************************************************************
 *                                                                                               *
 *                                Shooting Control Functions                                     *
 *                                                                                               *
 *************************************************************************************************/

void Shooter::Fire() noexcept
{
    firing = true;
}

void Shooter::Cease() noexcept
{
    firing = false;
}

float Shooter::ShiftIn(bool is_firing) noexcept
{
    firing = is_firing;
    Con->EnableSht(this);
    return speed;
}

bool Shooter::ShiftOut() noexcept
{
    bool ret = firing;
    firing = false;
    Con->DisableSht(this);
    return ret;
}

void Shooter::Sync()
{
}

void Shooter::FSync()
{
}

void Shooter::Heal(int curing)
{
    rate += curing;
}

bool Shooter::Hurt(int damage)
{
    rate -= damage;

    if (damage > 0)
    {
        if (rate > 0)
        {
        }
        else
        {
            return true;
        }
    }
    else if (damage < 0)
    {
    }

    return false;
}

void Shooter::Destroy()
{
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Shooting Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<void(Shooter *)> Shooter::patterns[static_cast<int>(SSPatternsCode::NUM)];

void Shooter::InitSSPattern()
{
    patterns[static_cast<int>(SSPatternsCode::CONTROLLED)] = std::mem_fn(&Shooter::controlled);
    patterns[static_cast<int>(SSPatternsCode::STAY)] = std::mem_fn(&Shooter::stay);
    patterns[static_cast<int>(SSPatternsCode::TRACK)] = std::mem_fn(&Shooter::track_player);
    patterns[static_cast<int>(SSPatternsCode::TOTAL_TURN)] = std::mem_fn(&Shooter::total_turn);
    patterns[static_cast<int>(SSPatternsCode::SPLIT_TURN)] = std::mem_fn(&Shooter::split_turn);
}

void Shooter::controlled()
{
}

void Shooter::stay()
{
    if (firing)
    {
        timer += 1;
        for (int s = 0; s < luncher_n; s++)
            if (timer % lunchers[s].Interval == 0)
                fire(s);
    }
    else
        timer = -1;
}

void Shooter::total_turn()
{
    my_angle += data.turn_speed;

    if (firing)
    {
        my_xf.q.Set(my_angle);

        timer += 1;
        for (int s = 0; s < luncher_n; s++)
            if (timer % lunchers[s].Interval == 0)
            {
                recalc_lunchers_attitude_cache(s);
                fire(s);
            }
    }
    else
        timer = -1;
}

void Shooter::split_turn()
{
    for (int s = 0; s < luncher_n; s++)
        lunchers[s].DAttitude.Angle += data.turn_speeds[s];

    if (firing)
    {
        timer += 1;

        for (int s = 0; s < luncher_n; s++)
            if (timer % lunchers[s].Interval == 0)
            {
                recalc_lunchers_attitude_cache(s);
                fire(s);
            }
    }
    else
        timer = -1;
}

void Shooter::track_enemy()
{
    if (firing)
    {
        bool imada = true;

        timer += 1;
        for (int s = 0; s < luncher_n; s++)
            if (timer % lunchers[s].Interval == 0)
            {
                if (imada)
                {
                    target = Con->TrackEnemy();
                    if (target != nullptr)
                        track();
                    imada = false;
                }
                recalc_lunchers_attitude_cache(s);
                fire(s);
            }
    }
    else
        timer = -1;
}

void Shooter::track_player()
{
    if (firing)
    {
        bool imada = true;

        timer += 1;
        for (int s = 0; s < luncher_n; s++)
            if (timer % lunchers[s].Interval == 0)
            {
                if (imada)
                {
                    target = Con->TrackEnemy();
                    track();
                    imada = false;
                }
                recalc_lunchers_attitude_cache(s);
                fire(s);
            }
    }
    else
        timer = -1;
}