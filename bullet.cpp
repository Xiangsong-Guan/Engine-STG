#include "bullet.h"

#include "resource_manger.h"

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

Bullet::Bullet()
{
    bd.type = b2_dynamicBody;
}

void Bullet::Load(const STGBulletSetting &bs, const b2Filter &f, b2World *w)
{
    /* Bullet's textures */
    if (bs.Texs.SpriteBornType == SpriteType::ANIMED)
        born = ResourceManager::GetAnime(bs.Texs.SpriteBorn);
    if (bs.Texs.SpriteDisableType == SpriteType::ANIMED)
        disable = ResourceManager::GetAnime(bs.Texs.SpriteDisable);
    if (bs.Texs.SpriteMovementType == SpriteType::ANIMED)
        idle = ResourceManager::GetAnime(bs.Texs.SpriteMovement[0]);
    else if (bs.Texs.SpriteMovementType == SpriteType::STATIC)
        idle = Anime(ResourceManager::GetTexture(bs.Texs.SpriteMovement[static_cast<int>(Movement::IDLE)]));
    else
        idle = Anime(ResourceManager::GetTexture("blank"));
    if (idle.DURATION == 1)
    {
        idle.Forward();
        idle.Forward();
        idle.Forward();
    }

    world = w;

    phy = bs.Phy;
    damage = bs.Damage;
    ks = bs.KS;

    phy.FD.density = bs.Density;
    phy.FD.filter = f;
    /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
    phy.FD.shape = phy.Shape == ShapeType::CIRCLE ? static_cast<b2Shape *>(&phy.C) : static_cast<b2Shape *>(&phy.P);

    if (ks.Track)
        /* Different side never use same bullet! */
        if (ks.Dir)
            pattern = phy.FD.filter.groupIndex == static_cast<int16>(CollisionType::G_ENEMY_SIDE)
                          ? std::mem_fn(&Bullet::track_player_d)
                          : std::mem_fn(&Bullet::track_enemy_d);
        else
            pattern = phy.FD.filter.groupIndex == static_cast<int16>(CollisionType::G_ENEMY_SIDE)
                          ? std::mem_fn(&Bullet::track_player)
                          : std::mem_fn(&Bullet::track_enemy);
    else if (ks.SeqSize == 1)
        if (ks.Dir)
            pattern = std::mem_fn(&Bullet::statical_d);
        else
            pattern = std::mem_fn(&Bullet::statical);
    else if (ks.Dir)
        pattern = std::mem_fn(&Bullet::kinematic_d);
    else
        pattern = std::mem_fn(&Bullet::kinematic);

    float final_angular_velocity = 0.f;
    for (int i = 0; i < ks.SeqSize; i++)
        final_angular_velocity += ks.Seq[i].VR;
    need_adjust_front_for_flt = (final_angular_velocity != 0.f);

    lost = true;
    just_bullets_n = 0;
    fly_bullets_n = 0;
    flt_bullets_n = 0;
    dead_bullets_n = 0;
}

void Bullet::SetScale(float x, float y, float phy, const float b[4]) noexcept
{
    std::memcpy(bound, b, sizeof(bound));
    draw.SetScale(x, y, phy);
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
        if (dead_bullets[i].ATimer < disable.LG_DURATION)
            dead_bullets[i].ATimer += 1;
        else
        {
            dead_bullets[i] = dead_bullets[dead_bullets_n - 1];
            dead_bullets_n -= 1;
        }
    }

    if (just_bullets_n + fly_bullets_n + flt_bullets_n + dead_bullets_n == 0)
    {
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
    if (disable.DURATION > 1)
        for (int i = 0; i < dead_bullets_n; i++)
            draw.Draw(dead_bullets[i].Physics, born.GetFrame(dead_bullets[i].ATimer));

    if (idle.DURATION > 1)
    {
        for (int i = 0; i < fly_bullets_n; i++)
            draw.Draw(fly_bullets[i].SB.Physics, born.GetFrame(fly_bullets[i].SB.ATimer));
        for (int i = 0; i < flt_bullets_n; i++)
            draw.Draw(flt_bullets[i].Physics, born.GetFrame(flt_bullets[i].ATimer));
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

    just_bullets[just_bullets_n].Physics = world->CreateBody(&bd);
    just_bullets[just_bullets_n].Physics->CreateFixture(&phy.FD);

    just_bullets[just_bullets_n].MasterPower = mp;
    just_bullets[just_bullets_n].ATimer = 0;
    just_bullets_n += 1;

    assert(just_bullets_n < MAX_JUST_BULLETS);

    if (lost)
    {
        lost = false;
        Con->EnableBullet(this);
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Bullets Update                                             *
 *                                                                                               *
 *************************************************************************************************/

inline void Bullet::kinematic_update(b2Body *b, const KinematicPhase &kp)
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    if (kp.VV != 0.f)
        if (kp.TransEnd == 0)
            b->ApplyLinearImpulseToCenter((kp.VV * b->GetMass()) * b->GetWorldVector(front), true);
        else
            // float a = kp.VV / kp.TransTime;
            // float f = b->GetMass() * a;
            b->ApplyForceToCenter((kp.VV / kp.TransTime) * b->GetMass() * b->GetWorldVector(front), true);

    if (kp.VR != 0.f)
        if (kp.TransEnd == 0.f)
            b->ApplyAngularImpulse(kp.VR * b->GetInertia(), true);
        else
            // float a = kp.VR / kp.TransTime;
            // float t = b->GetInertia() * a;
            b->ApplyTorque((kp.VR / kp.TransTime) * b->GetInertia(), true);
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
        {
            world->DestroyBody(flt_bullets[s].Physics);
            flt_bullets[s] = flt_bullets[flt_bullets_n - 1];
            flt_bullets_n -= 1;
        }
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
        {
            world->DestroyBody(flt_bullets[s].Physics);
            flt_bullets[s] = flt_bullets[flt_bullets_n - 1];
            flt_bullets_n -= 1;
        }
    }
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Different Patterns (non-d)                                 *
 *                                                                                               *
 *************************************************************************************************/

void Bullet::track_player()
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].Physics->ApplyLinearImpulseToCenter(
                ks.Seq[0].VV * just_bullets[i].Physics->GetMass() * just_bullets[i].Physics->GetWorldVector(front), true);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

    target = Con->TrackPlayer();
    for (int s = flt_bullets_n - 1; s >= 0; s--)
        track_update_nd(flt_bullets[s].Physics, target);

    check_flt_nd();
}

void Bullet::track_enemy()
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].Physics->ApplyLinearImpulseToCenter(
                ks.Seq[0].VV * just_bullets[i].Physics->GetMass() * just_bullets[i].Physics->GetWorldVector(front), true);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

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
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].ATimer = 0;
            fly_bullets[fly_bullets_n].SB = just_bullets[i];
            fly_bullets[fly_bullets_n].KSI = 0;
            fly_bullets[fly_bullets_n].KTimer = -1;
            fly_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(fly_bullets_n < MAX_D_BULLETS);
        }
    }

    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = fly_bullets[s].SB.Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
        {
            fly_bullets[s].SB.ATimer += 1;

            /* kinematic update */
            if (fly_bullets[s].KSI < ks.SeqSize)
            {
                fly_bullets[s].KTimer += 1;
                if (fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].TransEnd)
                    fly_bullets[s].KSI += 1;
                if (fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].PhaseTime &&
                    fly_bullets[s].KTimer < ks.Seq[fly_bullets[s].KSI].TransEnd)
                    kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI]);
            }
            else if (ks.Loop)
            {
                fly_bullets[s].KSI = 0;
                fly_bullets[s].KTimer = 0;
                kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI]);
                fly_bullets[s].KSI += 1;
            }
            else
            {
                flt_bullets[flt_bullets_n] = fly_bullets[s].SB;
                flt_bullets_n += 1;
                fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
                fly_bullets_n -= 1;

                assert(flt_bullets_n < MAX_S_BULLETS);
            }
        }
        else
        {
            world->DestroyBody(fly_bullets[s].SB.Physics);
            fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
            fly_bullets_n -= 1;
        }
    }

    check_flt_nd();
}

void Bullet::statical()
{
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            /* Only initial v. */
            kinematic_update(just_bullets[i].Physics, ks.Seq[0]);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

    check_flt_nd();
}

/*************************************************************************************************
 *                                                                                               *
 *                                    Different Patterns (d)                                     *
 *                                                                                               *
 *************************************************************************************************/

void Bullet::track_player_d()
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].Physics->ApplyLinearImpulseToCenter(
                ks.Seq[0].VV * just_bullets[i].Physics->GetMass() * just_bullets[i].Physics->GetWorldVector(front), true);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

    target = Con->TrackPlayer();
    for (int s = flt_bullets_n - 1; s >= 0; s--)
        track_update_dd(flt_bullets[s].Physics, target);

    check_flt_dd();
}

void Bullet::track_enemy_d()
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].Physics->ApplyLinearImpulseToCenter(
                ks.Seq[0].VV * just_bullets[i].Physics->GetMass() * just_bullets[i].Physics->GetWorldVector(front), true);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

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
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            just_bullets[i].ATimer = 0;
            fly_bullets[fly_bullets_n].SB = just_bullets[i];
            fly_bullets[fly_bullets_n].KSI = 0;
            fly_bullets[fly_bullets_n].KTimer = -1;
            fly_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(fly_bullets_n < MAX_D_BULLETS);
        }
    }

    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = fly_bullets[s].SB.Physics->GetPosition();
        if (p.y > bound[0] && p.y < bound[1] && p.x > bound[2] && p.x < bound[3])
        {
            fly_bullets[s].SB.ATimer += 1;

            /* kinematic update */
            if (fly_bullets[s].KSI < ks.SeqSize)
            {
                fly_bullets[s].KTimer += 1;
                if (fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].TransEnd)
                    fly_bullets[s].KSI += 1;
                if (fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].PhaseTime &&
                    fly_bullets[s].KTimer < ks.Seq[fly_bullets[s].KSI].TransEnd)
                    kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI]);

                adjust_front(fly_bullets[s].SB.Physics);
            }
            else if (ks.Loop)
            {
                fly_bullets[s].KSI = 0;
                fly_bullets[s].KTimer = 0;
                kinematic_update(fly_bullets[s].SB.Physics, ks.Seq[fly_bullets[s].KSI]);
                fly_bullets[s].KSI += 1;
            }
            else
            {
                flt_bullets[flt_bullets_n] = fly_bullets[s].SB;
                flt_bullets_n += 1;
                fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
                fly_bullets_n -= 1;

                assert(flt_bullets_n < MAX_S_BULLETS);
            }
        }
        else
        {
            world->DestroyBody(fly_bullets[s].SB.Physics);
            fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
            fly_bullets_n -= 1;
        }
    }

    check_flt_dd();
}

void Bullet::statical_d()
{
    for (int i = just_bullets_n - 1; i >= 0; i--)
    {
        if (just_bullets[i].ATimer < born.LG_DURATION)
            just_bullets[i].ATimer += 1;
        else
        {
            /* Only initial v. */
            kinematic_update(just_bullets[i].Physics, ks.Seq[0]);

            just_bullets[i].ATimer = 0;
            flt_bullets[flt_bullets_n] = just_bullets[i];
            flt_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;

            assert(flt_bullets_n < MAX_S_BULLETS);
        }
    }

    check_flt_dd();
}
