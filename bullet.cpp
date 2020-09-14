#include "bullet.h"

#include "resource_manger.h"

#include <iostream>

/*************************************************************************************************
 *                                                                                               *
 *                                Collision Proxy Function                                       *
 *                                                                                               *
 *************************************************************************************************/

void BulletCollisionHandler::Hit(CollisionHandler *o)
{
#ifdef _DEBUG
    std::cout << "Bullet-" << Master->CodeName << " hit!\n";
#endif

    o->Hurt(Master->GetChange(MyPower));
}

void BulletCollisionHandler::Hurt(const STGChange *c)
{
    Master->Disappear(this);
}

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

Bullet::Bullet()
{
    bd.type = b2_dynamicBody;
    /* During born animation, bullet does not collision or move. It will be wake up by force. */
    bd.awake = false;

    for (int i = 0; i < MAX_BULLETS; i++)
        collision_proxy[i].Master = this;

    bound[0] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[1] = PHYSICAL_HEIGHT + STG_FIELD_BOUND_BUFFER;
    bound[2] = 0.f - STG_FIELD_BOUND_BUFFER;
    bound[3] = PHYSICAL_WIDTH + STG_FIELD_BOUND_BUFFER;
}

void Bullet::Load(const STGBulletSetting &bs, const b2Filter &f, b2World *w)
{
    Name = bs.Name;
    CodeName = bs.CodeName;

    /* Bullet's textures */
    if (bs.Texs.SpriteBornType == SpriteType::SPT_ANIMED)
        born = ResourceManager::GetAnime(bs.Texs.SpriteBorn);
    else if (bs.Texs.SpriteBornType == SpriteType::SPT_STATIC)
        born = Anime(ResourceManager::GetTexture(bs.Texs.SpriteBorn));
    if (bs.Texs.SpriteHitType == SpriteType::SPT_ANIMED)
        hit = ResourceManager::GetAnime(bs.Texs.SpriteHit);
    else if (bs.Texs.SpriteHitType == SpriteType::SPT_STATIC)
        hit = Anime(ResourceManager::GetTexture(bs.Texs.SpriteHit));
    if (bs.Texs.SpriteMovementType == SpriteType::SPT_ANIMED)
        idle = ResourceManager::GetAnime(bs.Texs.SpriteMovement[0]);
    else if (bs.Texs.SpriteMovementType == SpriteType::SPT_STATIC)
        idle = Anime(ResourceManager::GetTexture(bs.Texs.SpriteMovement[Movement::MM_IDLE]));
    else
        idle = Anime(ResourceManager::GetTexture("blank"));
    if (idle.DURATION == 1)
        idle.Forward();
    if (hit.DURATION == 1)
        hit.Forward();
    if (born.DURATION == 1)
        born.Forward();

    /* Usully we count timer from -1, but here renderer takes frame asynchronously (FOR DEAD ONES). So conut from -1 will 
     * make error. Count from 0 and check in end of a frame will cause one frame more in the final end. That
     * is clearly visible. Minus one for total duration to avoid this happens. */
    hit.LG_DURATION -= 1;

    world = w;

    basic_power = bs.Change.any.Damage;
    change = bs.Change;
    phy = bs.Phy;
    ks = bs.KS;

    phy.FD.filter = f;
    /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
    phy.FD.shape = phy.Shape == ShapeType::ST_CIRCLE ? static_cast<b2Shape *>(&phy.C) : static_cast<b2Shape *>(&phy.P);

    if (ks.Track)
        /* Different side never use same bullet! */
        if (ks.Dir)
        {
#ifdef _DEBUG
            std::cout << "Bullet-" << CodeName << " Pattern: Track with dir. ";
#endif

            pattern = phy.FD.filter.groupIndex == CollisionType::G_ENEMY_SIDE
                          ? std::mem_fn(&Bullet::track_player_d)
                          : std::mem_fn(&Bullet::track_enemy_d);
        }
        else
        {
#ifdef _DEBUG
            std::cout << "Bullet-" << CodeName << " Pattern: Track without dir. ";
#endif

            pattern = phy.FD.filter.groupIndex == CollisionType::G_ENEMY_SIDE
                          ? std::mem_fn(&Bullet::track_player)
                          : std::mem_fn(&Bullet::track_enemy);
        }
    else if (ks.SeqSize == 1)
        if (ks.Dir)
        {
#ifdef _DEBUG
            std::cout << "Bullet-" << CodeName << " Pattern: Statical with dir. ";
#endif

            pattern = std::mem_fn(&Bullet::statical_d);
        }
        else
        {
#ifdef _DEBUG
            std::cout << "Bullet-" << CodeName << " Pattern: Statical without dir. ";
#endif

            pattern = std::mem_fn(&Bullet::statical);
        }
    else if (ks.Dir)
    {
#ifdef _DEBUG
        std::cout << "Bullet-" << CodeName << " Pattern: Kinematic with dir. ";
#endif

        pattern = std::mem_fn(&Bullet::kinematic_d);
    }
    else
    {
#ifdef _DEBUG
        std::cout << "Bullet-" << CodeName << " Pattern: Kinematic without dir. ";
#endif

        pattern = std::mem_fn(&Bullet::kinematic);
    }

    float final_angular_velocity = 0.f;
    for (int i = 0; i < ks.SeqSize; i++)
        final_angular_velocity += ks.Seq[i].VR;
    need_adjust_front_for_flt = (final_angular_velocity != 0.f);

#ifdef _DEBUG
    std::cout << "Final angular velocity: " << final_angular_velocity << "\n";
#endif

    reset_proxy();
    lost = true;
    just_bullets_n = 0;
    fly_bullets_n = 0;
    flt_bullets_n = 0;
    dead_bullets_n = 0;
}

/*************************************************************************************************
 *                                                                                               *
 *                                             Update                                            *
 *                                                                                               *
 *************************************************************************************************/

Bullet *Bullet::Update()
{
    pattern(this);

    for (int i = dead_bullets_n - 1; i >= 0; i--)
    {
        if (dead_bullets[i].ATimer < hit.LG_DURATION)
        {
            /* First loop for a dead bullet. Destroy its fixture to avoid further collision.
             * Also need hold up. */
            if (dead_bullets[i].ATimer == 0)
            {
                dead_bullets[i].Physics->SetLinearVelocity(b2Vec2_zero);
                dead_bullets[i].Physics->SetAngularVelocity(0.f);
                dead_bullets[i].Physics->SetAwake(false);
                dead_bullets[i].Physics->DestroyFixture(dead_bullets[i].Physics->GetFixtureList());
            }
            /* Note that (only) dead bullets count from 0. */
            dead_bullets[i].ATimer += 1;
        }
        else
        {
            world->DestroyBody(dead_bullets[i].Physics);
            if (i + 1 != dead_bullets_n)
                dead_bullets[i] = dead_bullets[dead_bullets_n - 1];
            dead_bullets_n -= 1;
        }
    }

    if (just_bullets_n + fly_bullets_n + flt_bullets_n + dead_bullets_n == 0)
    {
#ifdef _DEBUG
        std::cout << "Bullet-" << CodeName << " lost all!\n";
#endif

        lost = true;
        Con->DisableBullet(this);
    }

    return Next;
}

Bullet *Bullet::Draw(float forward_time)
{
    draw.Reset(forward_time);

    if (born.DURATION > 1)
        for (int i = 0; i < just_bullets_n; i++)
            draw.Draw(just_bullets[i].Physics, born.GetFrame(just_bullets[i].ATimer));
    else if (born.DURATION == 1)
        for (int i = 0; i < just_bullets_n; i++)
            draw.Draw(just_bullets[i].Physics, born.Playing);

    if (hit.DURATION > 1)
        for (int i = 0; i < dead_bullets_n; i++)
            draw.Draw(dead_bullets[i].Physics, hit.GetFrame(dead_bullets[i].ATimer));
    if (hit.DURATION == 1)
        for (int i = 0; i < dead_bullets_n; i++)
            draw.Draw(dead_bullets[i].Physics, hit.Playing);

    if (idle.DURATION > 1)
    {
        for (int i = 0; i < fly_bullets_n; i++)
            draw.Draw(fly_bullets[i].SB.Physics, idle.GetFrame(fly_bullets[i].SB.ATimer));
        for (int i = 0; i < flt_bullets_n; i++)
            draw.Draw(flt_bullets[i].Physics, idle.GetFrame(flt_bullets[i].ATimer));
    }
    else
    {
        for (int i = 0; i < fly_bullets_n; i++)
            draw.Draw(fly_bullets[i].SB.Physics, idle.Playing);
        for (int i = 0; i < flt_bullets_n; i++)
            draw.Draw(flt_bullets[i].Physics, idle.Playing);
    }

    draw.Flush();
    return Next;
}

void Bullet::Bang(const b2Vec2 &world_pos, float world_angle, int mp)
{
    bd.position = world_pos;
    bd.angle = world_angle;

    /* Leave fixture creatation when born animation end. */
    just_bullets[just_bullets_n].Physics = world->CreateBody(&bd);

    just_bullets[just_bullets_n].MasterPower = mp;
    just_bullets[just_bullets_n].ATimer = -1;
    just_bullets_n += 1;

    assert(just_bullets_n <= MAX_JUST_BULLETS);

    if (lost)
    {
#ifdef _DEBUG
        std::cout << "Bullet-" << CodeName << " works!\n";
#endif

        lost = false;
        Con->EnableBullet(this);
    }
}

/* ONLY called by the bullet's last master shooter. Usully used in stage finale. */
void Bullet::Farewell()
{
#ifdef _DEBUG
    std::cout << "Bullet-" << CodeName << ", Sayonara!\n";
#endif

    for (int i = 0; i < just_bullets_n; i++)
        world->DestroyBody(just_bullets[i].Physics);
    just_bullets_n = 0;
    for (int i = 0; i < fly_bullets_n; i++)
    {
        /* Note that dead bullets count from 0.*/
        return_proxy(reinterpret_cast<BulletCollisionHandler *>(fly_bullets[i].SB.Physics->GetUserData()) - collision_proxy);
        fly_bullets[i].SB.Physics->SetUserData(nullptr);

        if (dead_bullets_n < MAX_DEAD_BULLETS)
        {
            fly_bullets[i].SB.ATimer = 0;
            dead_bullets[dead_bullets_n] = fly_bullets[i].SB;
            dead_bullets_n += 1;
        }
        else
            world->DestroyBody(fly_bullets[i].SB.Physics);
    }
    fly_bullets_n = 0;
    for (int i = 0; i < flt_bullets_n; i++)
    {
        /* Note that dead bullets count from 0.*/
        return_proxy(reinterpret_cast<BulletCollisionHandler *>(flt_bullets[i].Physics->GetUserData()) - collision_proxy);
        flt_bullets[i].Physics->SetUserData(nullptr);

        if (dead_bullets_n < MAX_DEAD_BULLETS)
        {
            flt_bullets[i].ATimer = 0;
            dead_bullets[dead_bullets_n] = flt_bullets[i];
            dead_bullets_n += 1;
        }
        else
            world->DestroyBody(flt_bullets[i].Physics);
    }
    flt_bullets_n = 0;

    assert(dead_bullets_n <= MAX_DEAD_BULLETS);

#ifdef _DEBUG
    if (dead_bullets_n == MAX_DEAD_BULLETS)
        std::cout << "Bullet-" << CodeName << " not perfect sayonara!\n";
#endif
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Disable Things                                             *
 *                                                                                               *
 *************************************************************************************************/

STGChange *Bullet::GetChange(int mp)
{
    change.any.Damage = mp * basic_power;
    return &change;
}

/* DO NOT CHANGE PHYSICAL STATE HERE! LEAVE TO DEAD PROCESS IN UPDATE LOOP. */
void Bullet::Disappear(const BulletCollisionHandler *who)
{
#ifdef _DEBUG
    std::cout << "Bullet-" << CodeName << " hit one will lost!\n";
#endif

    const int flaw = who->PhysicsIndex;

    if (who->IsStatical)
    {
        /* Note that dead bullets count from 0.*/
        flt_bullets[flaw].ATimer = 0;
        flt_bullets[flaw].Physics->SetUserData(nullptr);

        dead_bullets[dead_bullets_n] = flt_bullets[flaw];

        if (flaw + 1 != flt_bullets_n)
        {
            flt_bullets[flaw] = flt_bullets[flt_bullets_n - 1];
            reinterpret_cast<BulletCollisionHandler *>(flt_bullets[flaw].Physics->GetUserData())->PhysicsIndex = flaw;
        }
        flt_bullets_n -= 1;
    }
    else
    {
        /* Note that dead bullets count from 0.*/
        fly_bullets[flaw].SB.ATimer = 0;
        fly_bullets[flaw].SB.Physics->SetUserData(nullptr);

        dead_bullets[dead_bullets_n] = fly_bullets[flaw].SB;

        if (flaw + 1 != fly_bullets_n)
        {
            fly_bullets[flaw] = fly_bullets[fly_bullets_n - 1];
            reinterpret_cast<BulletCollisionHandler *>(fly_bullets[flaw].SB.Physics->GetUserData())->PhysicsIndex = flaw;
        }
        fly_bullets_n -= 1;
    }

    return_proxy(who - collision_proxy);
    dead_bullets_n += 1;
    assert(dead_bullets_n <= MAX_DEAD_BULLETS);
}

/*************************************************************************************************
 *                                                                                               *
 *                                          Proxy Pool                                           *
 *                                                                                               *
 *************************************************************************************************/

inline int Bullet::get_proxy() noexcept
{
    for (int i = proxy_hint; i < MAX_BULLETS; i++)
    {
        if (!used_proxy[i])
        {
            proxy_hint = (i + 1) % MAX_BULLETS;
            used_proxy[i] = true;
            return i;
        }
    }

    proxy_hint = 0;
    for (int i = proxy_hint; i < MAX_BULLETS; i++)
    {
        if (!used_proxy[i])
        {
            proxy_hint = (i + 1) % MAX_BULLETS;
            used_proxy[i] = true;
            return i;
        }
    }

    assert(false);
    return 0x0fffffff;
}

inline void Bullet::reset_proxy() noexcept
{
    std::memset(used_proxy, 0, sizeof(used_proxy));
    proxy_hint = 0;
}

inline void Bullet::return_proxy(int id) noexcept
{
    used_proxy[id] = false;
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Define For BP/U                                            *
 *                                                                                               *
 *************************************************************************************************/

inline void Bullet::outsider_flt(int s)
{
    return_proxy(reinterpret_cast<BulletCollisionHandler *>(flt_bullets[s].Physics->GetUserData()) - collision_proxy);
    world->DestroyBody(flt_bullets[s].Physics);

    if (s + 1 != flt_bullets_n)
    {
        flt_bullets[s] = flt_bullets[flt_bullets_n - 1];
        reinterpret_cast<BulletCollisionHandler *>(flt_bullets[s].Physics->GetUserData())->PhysicsIndex = s;
    }
    flt_bullets_n -= 1;
}

inline void Bullet::outsider_fly(int s)
{
    return_proxy(reinterpret_cast<BulletCollisionHandler *>(fly_bullets[s].SB.Physics->GetUserData()) - collision_proxy);
    world->DestroyBody(fly_bullets[s].SB.Physics);

    if (s + 1 != fly_bullets_n)
    {
        fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
        reinterpret_cast<BulletCollisionHandler *>(fly_bullets[s].SB.Physics->GetUserData())->PhysicsIndex = s;
    }
    fly_bullets_n -= 1;
}

/* Only used before move just to fly/flt. */
inline void Bullet::make_cp_just(int i, bool is_statical, int n)
{
    int cp = get_proxy();
    collision_proxy[cp].IsStatical = is_statical;
    collision_proxy[cp].PhysicsIndex = n;
    collision_proxy[cp].MyPower = just_bullets[i].MasterPower;
    just_bullets[i].Physics->SetUserData(collision_proxy + cp);
    just_bullets[i].Physics->CreateFixture(&phy.FD);
}

inline void Bullet::move_from_fly_to_flt_with_cp(int s)
{
    auto cp = reinterpret_cast<BulletCollisionHandler *>(fly_bullets[s].SB.Physics->GetUserData());
    cp->IsStatical = true;
    cp->PhysicsIndex = flt_bullets_n;

    flt_bullets[flt_bullets_n] = fly_bullets[s].SB;
    flt_bullets_n += 1;
    fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
    fly_bullets_n -= 1;

    assert(flt_bullets_n <= MAX_BULLETS);
}

inline void Bullet::move_from_just_to_flt(int i)
{
    just_bullets[i].ATimer = -1;
    flt_bullets[flt_bullets_n] = just_bullets[i];
    flt_bullets_n += 1;
    just_bullets[i] = just_bullets[just_bullets_n - 1];
    just_bullets_n -= 1;

    assert(flt_bullets_n <= MAX_BULLETS);
}

inline void Bullet::move_from_just_to_fly(int i)
{
    just_bullets[i].ATimer = -1;
    fly_bullets[fly_bullets_n].SB = just_bullets[i];
    fly_bullets[fly_bullets_n].KSI = 0;
    fly_bullets[fly_bullets_n].KTimer = -1;
    fly_bullets_n += 1;
    just_bullets[i] = just_bullets[just_bullets_n - 1];
    just_bullets_n -= 1;

    assert(fly_bullets_n <= MAX_BULLETS);
}

inline void Bullet::kinematic_a_part()
{
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        just_bullets[i].ATimer += 1;
        if (just_bullets[i].ATimer == born.LG_DURATION)
        {
            make_cp_just(i, false, fly_bullets_n);
            /* Front always be init front, for not-dir. */
            just_bullets[i].Front = just_bullets[i].Physics->GetWorldVector(b2Vec2(0.f, -1.f));
            move_from_just_to_fly(i);
        }
    }
}

inline void Bullet::kinematic_d_part()
{
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        just_bullets[i].ATimer += 1;
        if (just_bullets[i].ATimer == born.LG_DURATION)
        {
            make_cp_just(i, false, fly_bullets_n);
            move_from_just_to_fly(i);
        }
    }
}

inline void Bullet::statical_a_part()
{
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        just_bullets[i].ATimer += 1;
        if (just_bullets[i].ATimer == born.LG_DURATION)
        {
            make_cp_just(i, true, flt_bullets_n);
            /* Only initial v. */
            kinematic_update(just_bullets[i].Physics, ks.Seq[0],
                             just_bullets[i].Physics->GetWorldVector(b2Vec2(0.f, -1.f)));
            move_from_just_to_flt(i);
        }
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Bullets Update                                             *
 *                                                                                               *
 *************************************************************************************************/

inline void Bullet::kinematic_update(b2Body *b, const KinematicPhase &kp, const b2Vec2 &front)
{
    if (kp.VV != 0.f)
        if (kp.VAcceleration == 0)
            b->ApplyLinearImpulseToCenter((kp.VV * b->GetMass()) * front, true);
        else
            // float a = kp.VV / kp.TransTime;
            // float f = b->GetMass() * a;
            b->ApplyForceToCenter(kp.VAcceleration * b->GetMass() * front, true);

    if (kp.VR != 0.f)
        if (kp.RAcceleration == 0.f)
            b->ApplyAngularImpulse(kp.VR * b->GetInertia(), true);
        else
            // float a = kp.VR / kp.TransTime;
            // float t = b->GetInertia() * a;
            b->ApplyTorque(kp.RAcceleration * b->GetInertia(), true);
}

inline void Bullet::track_update_nd(b2Body *s, const b2Body *d)
{
    b2Vec2 v_front = s->GetLinearVelocity();
    v_front.Normalize();
    b2Vec2 world_diff = d->GetPosition() - s->GetPosition();
    world_diff.Normalize();

    b2Rot dr;
    dr.c = b2Dot(world_diff, v_front);
    dr.s = -b2Cross(world_diff, v_front);

    s->SetLinearVelocity(ks.Seq[0].VV * b2Mul(dr, v_front));
}

inline void Bullet::track_update_dd(b2Body *s, const b2Body *d)
{
    static constexpr float EP = .01f;

    const b2Vec2 diff = s->GetLocalPoint(d->GetPosition());

    if (diff.x > EP)
        s->SetAngularVelocity(ks.Seq[0].VR);
    else if (diff.x < -EP)
        s->SetAngularVelocity(-ks.Seq[0].VR);
    else if (diff.y > 0.f)
        s->SetAngularVelocity(ks.Seq[0].VR);
    else
        s->SetAngularVelocity(0.f);
}

/* If bullet is rotting, need to update its vec dir to make it looks like go front always. */
inline void Bullet::adjust_front(b2Body *b)
{
    if (b->GetAngularVelocity() == 0.f)
        return;

    const b2Vec2 front = b2Vec2(0.f, -1.f);

    b2Vec2 v = b->GetLinearVelocity();
    b2Vec2 nv = b->GetWorldVector(front);
    v.Normalize();

    b2Rot dr;
    dr.c = b2Dot(nv, v);
    dr.s = -b2Cross(nv, v);

    b->SetLinearVelocity(b2Mul(dr, b->GetLinearVelocity()));
}

inline void Bullet::check_flt_dd()
{
    for (int s = flt_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = flt_bullets[s].Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
        {
            if (need_adjust_front_for_flt)
                adjust_front(flt_bullets[s].Physics);
            flt_bullets[s].ATimer += 1;
        }
        else
            outsider_flt(s);
    }
}

inline void Bullet::check_flt_nd()
{
    for (int s = flt_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = flt_bullets[s].Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
            flt_bullets[s].ATimer += 1;
        else
            outsider_flt(s);
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Different Patterns (non-d)                                 *
 *                                                                                               *
 *************************************************************************************************/

void Bullet::track_player()
{
    statical_a_part();

    target = Con->TrackPlayer();
    for (int s = flt_bullets_n - 1; s >= 0; s--)
        track_update_nd(flt_bullets[s].Physics, target);

    check_flt_nd();
}

void Bullet::track_enemy()
{
    statical_a_part();

    target = Con->TrackEnemy();
    if (target != nullptr)
        for (int s = flt_bullets_n - 1; s >= 0; s--)
            track_update_nd(flt_bullets[s].Physics, target);
    else
        for (int s = flt_bullets_n - 1; s >= 0; s--)
            flt_bullets[s].Physics->SetAngularVelocity(0.f);

    check_flt_nd();
}

void Bullet::kinematic()
{
    kinematic_a_part();

    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = fly_bullets[s].SB.Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
        {
            fly_bullets[s].SB.ATimer += 1;

            /* kinematic update */
            fly_bullets[s].KTimer += 1;

            if ((fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].PhaseTime &&
                 fly_bullets[s].KTimer < ks.Seq[fly_bullets[s].KSI].TransEnd) ||
                fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI], fly_bullets[s].SB.Front);

            while (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].TransEnd)
            {
                fly_bullets[s].KSI += 1;

                if (fly_bullets[s].KSI < ks.SeqSize)
                {
                    if (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                        kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI], fly_bullets[s].SB.Front);
                }
                else if (ks.Loop)
                {
                    fly_bullets[s].KSI = 0;
                    fly_bullets[s].KTimer = 0;
                    if (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                        kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI], fly_bullets[s].SB.Front);
                }
                else
                    move_from_fly_to_flt_with_cp(s);
            }
        }
        else
            outsider_fly(s);
    }

    check_flt_nd();
}

void Bullet::statical()
{
    statical_a_part();
    check_flt_nd();
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Different Patterns (d)                                     *
 *                                                                                               *
 *************************************************************************************************/

void Bullet::track_player_d()
{
    statical_a_part();

    target = Con->TrackPlayer();
    for (int s = flt_bullets_n - 1; s >= 0; s--)
        track_update_dd(flt_bullets[s].Physics, target);

    check_flt_dd();
}

void Bullet::track_enemy_d()
{
    statical_a_part();

    target = Con->TrackEnemy();
    if (target != nullptr)
        for (int s = flt_bullets_n - 1; s >= 0; s--)
            track_update_dd(flt_bullets[s].Physics, target);
    else
        for (int s = flt_bullets_n - 1; s >= 0; s--)
            flt_bullets[s].Physics->SetAngularVelocity(0.f);

    check_flt_dd();
}

void Bullet::kinematic_d()
{
    kinematic_d_part();

    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = fly_bullets[s].SB.Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
        {
            bool no_more = false;

            /* kinematic update */
            fly_bullets[s].KTimer += 1;

            if ((fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].PhaseTime &&
                 fly_bullets[s].KTimer < ks.Seq[fly_bullets[s].KSI].TransEnd) ||
                fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI],
                                 fly_bullets[s].SB.Physics->GetWorldVector(b2Vec2(0.f, -1.f)));

            while (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].TransEnd)
            {
                fly_bullets[s].KSI += 1;

                if (fly_bullets[s].KSI < ks.SeqSize)
                {
                    if (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                        kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI],
                                         fly_bullets[s].SB.Physics->GetWorldVector(b2Vec2(0.f, -1.f)));
                }
                else if (ks.Loop)
                {
                    fly_bullets[s].KSI = 0;
                    fly_bullets[s].KTimer = 0;
                    if (fly_bullets[s].KTimer == ks.Seq[fly_bullets[s].KSI].PhaseTime)
                        kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI],
                                         fly_bullets[s].SB.Physics->GetWorldVector(b2Vec2(0.f, -1.f)));
                }
                else
                    no_more = true;
            }

            if (no_more)
                move_from_fly_to_flt_with_cp(s);
            else
            {
                fly_bullets[s].SB.ATimer += 1;
                adjust_front(fly_bullets[s].SB.Physics);
            }
        }
        else
            outsider_fly(s);
    }

    check_flt_dd();
}

void Bullet::statical_d()
{
    statical_a_part();
    check_flt_dd();
}
