#include "stg_shooter.h"

#include "resource_manger.h"
#include "game_event.h"

#include <lua.hpp>

#include <iostream>

STGShooter::STGShooter()
{
    bd.type = b2_dynamicBody;
}

STGShooter &STGShooter::operator=(const STGShooter &o)
{
    std::memcpy(bound, o.bound, sizeof(bound));
    my_xf = b2Transform(b2Vec2_zero, b2Rot(0.f));
    my_atti = {b2Vec2_zero, 0.f};
    Name = o.Name;
    power = o.power;
    speed = o.speed;
    ammo_slot_n = o.ammo_slot_n;
    luncher_n = o.luncher_n;

    std::memcpy(lunchers, o.lunchers, sizeof(Luncher) * luncher_n);
    std::memcpy(bullets, o.bullets, sizeof(Bullet) * ammo_slot_n);
    for (int s = 0; s < ammo_slot_n; s++)
        /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
        bullets[s].Phy.FD.shape = bullets[s].Phy.Shape == ShapeType::CIRCLE
                                      ? static_cast<b2Shape *>(&bullets[s].Phy.C)
                                      : static_cast<b2Shape *>(&bullets[s].Phy.P);

    Shift = nullptr;
    Prev = nullptr;
    Next = nullptr;
    timer = -1;
    s_bullet_n = 0;
    d_bullet_n = 0;
    firing = false;
    formation = false;
    rate = 100;

    code = o.code;
    pattern = patterns[static_cast<int>(code)];
    data = o.data;
    if (code == SSPatternsCode::CONTROLLED)
    {
        int good, rn;

        data.AI = ResourceManager::GetCoroutine(ResourceManager::STG_SHOOT_FUNCTIONS_KEY, Name);
        good = lua_resume(data.AI, nullptr, 0, &rn);

#ifdef STG_LUA_API_ARG_CHECK
        if (good == LUA_OK)
            std::cerr << "STG shooter" << Name << " lua error: return in init call!" << std::endl;
        else if (good != LUA_YIELD)
            std::cerr << "STG shooterr" << Name << " lua error: " << lua_tostring(data.AI, -1) << std::endl;
        else if (rn != 0)
            std::cerr << "STG shooterr" << Name << " lua return something stupid!\n";
#endif
    }

    return *this;
}

void STGShooter::Load(const float b[4], const STGShooterSetting &setting, std::queue<STGBulletSetting> &bss)
{
    std::memcpy(bound, b, sizeof(bound));
    my_xf = b2Transform(b2Vec2_zero, b2Rot(0.f));
    my_atti = {b2Vec2_zero, 0.f};
    Name = setting.Name;
    power = setting.Power;
    speed = setting.Speed;
    ammo_slot_n = setting.AmmoSlotsNum;
    luncher_n = setting.LuncherSize;

    for (int s = 0; s < luncher_n; s++)
        lunchers[s] = setting.Lunchers[s];

    for (int s = 0; s < ammo_slot_n; s++)
    {
        bullets[s] = {bss.front().Speed, bss.front().Damage, bss.front().KS, bss.front().Phy};
        /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
        bullets[s].Phy.FD.shape = bullets[s].Phy.Shape == ShapeType::CIRCLE
                                      ? static_cast<b2Shape *>(&bullets[s].Phy.C)
                                      : static_cast<b2Shape *>(&bullets[s].Phy.P);
        if (bss.size() > 1)
            bss.pop();
    }

    Shift = nullptr;
    Prev = nullptr;
    Next = nullptr;
    timer = -1;
    s_bullet_n = 0;
    d_bullet_n = 0;
    firing = false;
    formation = false;
    rate = 100;

    code = setting.Pattern;
    pattern = patterns[static_cast<int>(code)];
    data = setting.Data;
    if (code == SSPatternsCode::CONTROLLED)
    {
        int good, rn;

        data.AI = ResourceManager::GetCoroutine(ResourceManager::STG_SHOOT_FUNCTIONS_KEY, Name);
        good = lua_resume(data.AI, nullptr, 0, &rn);

#ifdef STG_LUA_API_ARG_CHECK
        if (good == LUA_OK)
            std::cerr << "STG shooterr" << Name << " lua error: return in init call!" << std::endl;
        else if (good != LUA_YIELD)
            std::cerr << "STG shooterr" << Name << " lua error: " << lua_tostring(data.AI, -1) << std::endl;
        else if (rn != 0)
            std::cerr << "STG shooterr" << Name << " lua return something stupid!\n";
#endif
    }
}

STGShooter *STGShooter::Undershift(int id, const b2Body *body, b2World *w, ALLEGRO_EVENT_SOURCE *rm) noexcept
{
    ID = id;
    world = w;
    physical = body;
    render_master = rm;

    return Shift;
}

STGShooter *STGShooter::Update()
{
    /* Do pattern or lua for luncher. No fire no movement. */
    if (firing)
        pattern(this);

    for (int s = d_bullet_n - 1; s >= 0; s--)
    {
        b2Vec2 p = d_bullets[s].Physics->GetPosition();
        if (p.y < bound[0] || p.y > bound[1] || p.x < bound[2] || p.x > bound[3])
        {
            world->DestroyBody(d_bullets[s].Physics);
            d_bullets[s] = d_bullets[d_bullet_n - 1];
            d_bullet_n -= 1;
        }
        else
        {
            update_phy(s);

            // move to s}
        }
    }

    for (int s = s_bullet_n - 1; s >= 0; s--)
    {
        b2Vec2 p = s_bullets[s]->GetPosition();
        if (p.y < bound[0] || p.y > bound[1] || p.x < bound[2] || p.x > bound[3])
        {
            world->DestroyBody(s_bullets[s]);
            s_bullets[s] = s_bullets[s_bullet_n - 1];
            s_bullet_n -= 1;
        }
    }

    if (!formation && d_bullet_n == 0)
        Con->DisableSht(ID, this);

    return Next;
}

inline void STGShooter::update_phy(int s)
{
}

inline void STGShooter::update_my_attitude() noexcept
{
}

inline void STGShooter::update_luncher_attitude(int s) noexcept
{
}

inline void STGShooter::update_attitude(Attitude &a, const KinematicPhase &k) noexcept
{
}

inline void STGShooter::fire(int s)
{
    int ams = lunchers[s].AmmoSlot;

    /* Charactor will never rot, use pos instead of GetWorldPoint will be more efficiency. */
    bd.position = physical->GetPosition() + my_atti.Pos + lunchers[s].DAttitude.Pos;
    /* Charactor will never rot, no need to plus charactor's angle. */
    bd.angle = my_atti.Angle + lunchers[s].DAttitude.Angle;

    b2Body *b = world->CreateBody(&bd);
    b->CreateFixture(&bullets[ams].Phy.FD);

    b2Vec2 imp = b->GetWorldVector(b2Vec2(0.f, -bullets[ams].Speed * b->GetMass()));
    b->ApplyLinearImpulseToCenter(imp, true);

    if (bullets[ams].KS.Stay)
    {
        s_bullets[s_bullet_n] = b;
        s_bullet_n += 1;
    }
    else
    {
        d_bullets[d_bullet_n] = {0, 0, ams, b};
        d_bullet_n += 1;
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                Shooting Control Functions                                     *
 *                                                                                               *
 *************************************************************************************************/

void STGShooter::Fire() noexcept
{
    firing = true;
}

void STGShooter::Cease() noexcept
{
    firing = false;
}

float STGShooter::ShiftIn() noexcept
{
    formation = true;
    Con->EnableSht(ID, this);
    return speed;
}

void STGShooter::ShiftOut() noexcept
{
    formation = false;
}

void STGShooter::Sync()
{
}

void STGShooter::FSync()
{
}

void STGShooter::Destroy()
{
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Shooting Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<void(STGShooter *)> STGShooter::patterns[static_cast<int>(SSPatternsCode::NUM)];

void STGShooter::InitSSPattern()
{
    patterns[static_cast<int>(SSPatternsCode::TOTAL_TURN)] = std::mem_fn(&STGShooter::total_turn);
    patterns[static_cast<int>(SSPatternsCode::CONTROLLED)] = std::mem_fn(&STGShooter::controlled);
    patterns[static_cast<int>(SSPatternsCode::STAY)] = std::mem_fn(&STGShooter::stay);
}

void STGShooter::controlled()
{
}

void STGShooter::stay()
{
    timer += 1;

    for (int s = 0; s < luncher_n; s++)
        if (timer % lunchers[s].Interval == 0)
            fire(s);
}

void STGShooter::total_turn()
{
}