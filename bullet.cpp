#include "bullet.h"

Bullet::Bullet()
{
    bd.type = b2_dynamicBody;
}

void Bullet::Load(const STGBulletSetting &bs, const b2Filter &f, b2World *w)
{
    world = w;

    phy = bs.Phy;
    damage = bs.Damage;
    ks = bs.KS;

    phy.FD.density = 1.f;
    phy.FD.filter = f;
    /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
    phy.FD.shape = phy.Shape == ShapeType::CIRCLE ? static_cast<b2Shape *>(&phy.C) : static_cast<b2Shape *>(&phy.P);

    if (ks.Track)
        pattern = phy.FD.filter.groupIndex == static_cast<int16>(CollisionType::G_ENEMY_SIDE)
                      ? patterns[static_cast<int>(SBPatternCode::TRACK_PLAYER)]
                      : patterns[static_cast<int>(SBPatternCode::TRACK_ENEMY)];
    else if (ks.SeqSize == 1)
        pattern = patterns[static_cast<int>(SBPatternCode::STATIC)];
    else
        pattern = patterns[static_cast<int>(SBPatternCode::KINEMATIC)];

    float final_angular_velocity = 0.f;
    for (int i = 0; i < ks.SeqSize; i++)
        final_angular_velocity += ks.Seq[i].VR;
    need_adjust_front_for_flt = (final_angular_velocity != 0.f) && ks.Dir;

    lost = true;
    just_bullets_n = 0;
    fly_bullets_n = 0;
    flt_bullets_n = 0;
    dead_bullets_n = 0;
}

void Bullet::SetScale(float x, float y, float phy, const float b[4]) noexcept
{
    std::memcpy(bound, b, sizeof(bound));
    // draw.SetScale(x, y, phy);
}

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

    if (lost)
    {
        lost = false;
        Con->EnableBullet(this);
    }
}

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

inline void Bullet::track_update(b2Body *s, const b2Body *d)
{
}

/* If bullet is rotting, need to update its vec dir to make it looks like go front always. */
inline void Bullet::adjust_front(b2Body *b)
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);
    b2Vec2 v = b->GetLinearVelocity();
    b2Vec2 nv = b->GetWorldVector(front);
    v.Normalize();
    b2Rot dr;
    dr.c = b2Dot(nv, v);
    dr.s = -b2Cross(nv, v);

    b->SetLinearVelocity(b2Mul(dr, b->GetLinearVelocity()));
}

inline void Bullet::check_fly()
{
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

                if (ks.Dir && fly_bullets[s].SB.Physics->GetAngularVelocity() != 0.f)
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
            }
        }
        else
        {
            world->DestroyBody(fly_bullets[s].SB.Physics);
            fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
            fly_bullets_n -= 1;
        }
    }
}

inline void Bullet::check_flt()
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

std::function<void(Bullet *)> Bullet::patterns[static_cast<int>(SBPatternCode::NUM)];

void Bullet::track_player()
{
    target = Con->TrackPlayer();
    if (target != nullptr)
        for (int s = fly_bullets_n - 1; s >= 0; s--)
            track_update(fly_bullets[s].SB.Physics, target);
}

void Bullet::track_enemy()
{
    target = Con->TrackEnemy();
    if (target != nullptr)
        for (int s = fly_bullets_n - 1; s >= 0; s--)
            track_update(fly_bullets[s].SB.Physics, target);
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
        }
    }

    check_fly();

    check_flt();
}

void Bullet::statical()
{
    const b2Vec2 front = b2Vec2(0.f, -1.f);

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
        }
    }

    check_flt();
}

void Bullet::InitBulletPatterns()
{
    patterns[static_cast<int>(SBPatternCode::STATIC)] = std::mem_fn(&Bullet::statical);
    patterns[static_cast<int>(SBPatternCode::KINEMATIC)] = std::mem_fn(&Bullet::kinematic);
    patterns[static_cast<int>(SBPatternCode::TRACK_ENEMY)] = std::mem_fn(&Bullet::track_enemy);
    patterns[static_cast<int>(SBPatternCode::TRACK_PLAYER)] = std::mem_fn(&Bullet::track_player);
}