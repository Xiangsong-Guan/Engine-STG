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

    phy.FD.filter = f;
    /* FD will loose shape, it just store its pointer. COPY WILL HAPPEN ONLY WHEN CREATION! */
    phy.FD.shape = phy.Shape == ShapeType::CIRCLE ? static_cast<b2Shape *>(&phy.C) : static_cast<b2Shape *>(&phy.P);

    if (ks.Track)
        pattern = phy.FD.filter.groupIndex == static_cast<int16>(CollisionType::G_ENEMY_SIDE)
                      ? patterns[static_cast<int>(STGBulletPatternCode::TRACK_PLAYER)]
                      : patterns[static_cast<int>(STGBulletPatternCode::TRACK_ENEMY)];
    else if (ks.SeqSize == 1)
        pattern = patterns[static_cast<int>(STGBulletPatternCode::STATIC)];
    else
        pattern = patterns[static_cast<int>(STGBulletPatternCode::KINEMATIC)];

    lost = true;
}

void Bullet::SetScale(float x, float y, float phy, const float b[4]) noexcept
{
    std::memcpy(bound, b, sizeof(bound));
    // draw.SetScale(x, y, phy);
}

Bullet *Bullet::Update()
{
    check_outside();
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

    if (just_bullets_n + fly_bullets_n + dead_bullets_n == 0)
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

void Bullet::kinematic_update(const DynamicBullet &db, const KinematicPhase &kp)
{
}

void Bullet::track_update(b2Body *s, const b2Body *d)
{
}

void Bullet::check_outside()
{
    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        const b2Vec2 &p = fly_bullets[s].SB.Physics->GetPosition();
        if (p.y < bound[0] || p.y > bound[1] || p.x < bound[2] || p.x > bound[3])
        {
            world->DestroyBody(fly_bullets[s].SB.Physics);
            fly_bullets[s] = fly_bullets[fly_bullets_n - 1];
            fly_bullets_n -= 1;
        }
    }
}

std::function<void(Bullet *)> Bullet::patterns[static_cast<int>(STGBulletPatternCode::NUM)];

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

    for (int s = fly_bullets_n - 1; s >= 0; s--)
    {
        fly_bullets[s].SB.ATimer += 1;

        /* kinematic update */
        if (fly_bullets[s].KSI < ks.SeqSize)
        {
            fly_bullets[s].KTimer += 1;
            if (fly_bullets[s].KTimer >= ks.Seq[fly_bullets[s].KSI].PhaseTime)
                kinematic_update(fly_bullets[s], ks.Seq[fly_bullets[s].KSI]);
            fly_bullets[s].KSI += 1;
        }
        else if (ks.Loop)
        {
            fly_bullets[s].KSI = 0;
            fly_bullets[s].KTimer = 0;
            kinematic_update(fly_bullets[s], ks.Seq[fly_bullets[s].KSI]);
            fly_bullets[s].KSI += 1;
        }
    }
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
            b2Vec2 imp = (ks.Seq[0].VV * just_bullets[i].Physics->GetMass()) * just_bullets[i].Physics->GetWorldVector(front);
            just_bullets[i].Physics->ApplyLinearImpulseToCenter(imp, true);
            if (ks.Seq[0].VR != 0.f)
                just_bullets[i].Physics->ApplyAngularImpulse(ks.Seq[0].VR * just_bullets[i].Physics->GetInertia(), true);

            just_bullets[i].ATimer = 0;
            fly_bullets[fly_bullets_n].SB = just_bullets[i];
            fly_bullets_n += 1;
            just_bullets[i] = just_bullets[just_bullets_n - 1];
            just_bullets_n -= 1;
        }
    }

    for (int s = fly_bullets_n - 1; s >= 0; s--)
        fly_bullets[s].SB.ATimer += 1;
}

void Bullet::InitBulletPatterns()
{
    patterns[static_cast<int>(STGBulletPatternCode::STATIC)] = std::mem_fn(&Bullet::statical);
    patterns[static_cast<int>(STGBulletPatternCode::KINEMATIC)] = std::mem_fn(&Bullet::kinematic);
    patterns[static_cast<int>(STGBulletPatternCode::TRACK_ENEMY)] = std::mem_fn(&Bullet::track_enemy);
    patterns[static_cast<int>(STGBulletPatternCode::TRACK_PLAYER)] = std::mem_fn(&Bullet::track_player);
}