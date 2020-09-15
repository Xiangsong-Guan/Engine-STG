#include "shooter.h"

#include "resource_manger.h"

#include <lua.hpp>

#include <iostream>

inline void Shooter::init()
{
    my_xf = b2Transform(b2Vec2_zero, b2Rot(0.f));
    my_angle = 0.f;

    timer = -1;
    firing = false;
    Chain = nullptr;
    rate = 100;

    sub_ptn = SSPatternsCode::SSPC_CONTROLLED;
    if (code == SSPatternsCode::SSPC_CONTROLLED)
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
    pattern = patterns[code];
    data = o.data;

#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " copy with pattern: " << SS_PATTERNS_CODE[code] << "\n";
#endif

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
    pattern = patterns[code];
    data = setting.Data;

#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " load with pattern: " << SS_PATTERNS_CODE[code] << "\n";
#endif

    init();
}

Shooter *Shooter::Undershift(const b2Body *body) noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " undershift. Up: " << ShiftUp->CodeName << "; Down: " << ShiftDown->CodeName << ".\n";
#endif

    physical = body;

    return ShiftDown;
}

/* Player are special, need get target from world. */
void Shooter::MyDearPlayer() noexcept
{
    if (code == SSPatternsCode::SSPC_TRACK)
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

bool Shooter::IsFiring() const noexcept
{
    return firing;
}

void Shooter::SetFire(bool f) noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " set fire: " << f << "!\n";
#endif

    firing = f;

    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " pass command to chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        Chain->SetFire(f);
    }
}

float Shooter::ShiftIn() noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " standby!\n";
#endif

    Con->EnableSht(this);

    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " pass command to chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        /* Return the slowest speed of the chain. */
        float ns = Chain->ShiftIn();
        return speed > ns ? ns : speed;
    }

    return speed;
}

/* Automaitc set the "ret"'s fire state to the same (when needed). */
Shooter *Shooter::ShiftOut(bool need_ret) noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " unload!\n";
#endif

    Shooter *ret = this;
    bool f = firing;

    Con->DisableSht(this);
    firing = false;

    /* Handout the end of chain's shift. */
    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " pass command to chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        ret = Chain->ShiftOut(need_ret);
    }
    else if (need_ret)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " automatic set shift down's fire: " << f << "\n";
#endif

        ret = ShiftDown;
        /* Set the same state to the new shooter. */
        ret->SetFire(f);

#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " return " << ret->CodeName << " for the chain.\n";
#endif
    }

    return ret;
}

/* Also shift in all chained shooters with the same firing state. */
void Shooter::LinkChain(int n) noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " chain-" << n << "!\n";
#endif

    if (n > 1) /* last 2, self and the next one */
    {
        /* Avoid circle chain. */
        if (ShiftDown->Chain != nullptr || ShiftDown == this || Chain != nullptr)
        {
#ifdef _DEBUG
            std::cout << "Shooter-" << CodeName << " chain to chained. Stop!\n";
#endif

            return;
        }

        Chain = ShiftDown;

#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " continue to chain: " << Chain->CodeName << ".\n";
#endif

        Chain->ShiftIn();
        Chain->SetFire(firing);
        Chain->LinkChain(n - 1);
    }
#ifdef _DEBUG
    else
        std::cout << "Shooter-" << CodeName << " last shooter on the chain!\n";
#endif
}

/* Also shift out all shooters, expect the head one. Drop all disconnected. */
int Shooter::Breakchain() noexcept
{
#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " will break the chain!\n";
#endif

    int n = 0;

    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " continue to break chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        n = Chain->Breakchain();

#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " last " << n << " shooters still.\n";
#endif

        /* Do not bother others when break the chain. */
        Chain->ShiftOut(false);
        Chain = nullptr;
    }

    if (rate > 0)
        return n + 1; /* self */
    else
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " disconnect from the chain, lost!\n";
#endif

        ShiftDown->ShiftUp = ShiftUp;
        ShiftUp->ShiftDown = ShiftDown;
        return n;
    }
}

bool Shooter::IsConnected() const noexcept
{
    return rate > 0;
}

void Shooter::Heal(int curing) noexcept
{
    rate += curing;

#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " take cure:" << curing << "! Now last: " << rate << ".\n";
#endif

    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " pass cure to chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        Chain->Heal(curing);
    }
}

/* Return true if shooter is disconnected. When chained, if any shooter disconnects, also return true. */
bool Shooter::Hurt(int damage) noexcept
{
    bool broken = false;

    rate -= damage;

#ifdef _DEBUG
    std::cout << "Shooter-" << CodeName << " take damage:" << damage << "! Now last: " << rate << ".\n";
#endif

    if (Chain != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " pass damage to chain: " << Chain->CodeName << ".\n";
#endif
        assert(Chain == ShiftDown);

        broken = Chain->Hurt(damage);
    }

    if (rate < 1)
    {
#ifdef _DEBUG
        std::cout << "Shooter-" << CodeName << " disconnect!\n";
#endif

        return true;
    }
    else
        return broken;
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Shooting Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<void(Shooter *)> Shooter::patterns[SSPatternsCode::SSPC_NUM];

void Shooter::InitSSPattern()
{
    patterns[SSPatternsCode::SSPC_CONTROLLED] = std::mem_fn(&Shooter::controlled);
    patterns[SSPatternsCode::SSPC_STAY] = std::mem_fn(&Shooter::stay);
    patterns[SSPatternsCode::SSPC_TRACK] = std::mem_fn(&Shooter::track_player);
    patterns[SSPatternsCode::SSPC_TOTAL_TURN] = std::mem_fn(&Shooter::total_turn);
    patterns[SSPatternsCode::SSPC_SPLIT_TURN] = std::mem_fn(&Shooter::split_turn);
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
                    target = Con->TrackPlayer();
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