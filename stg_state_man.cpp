#include "stg_state_man.h"

#include "resource_manger.h"

STGStateMan::STGStateMan()
{
    sbp[static_cast<int>(SpriteType::STATIC)] = born_s;
    sbp[static_cast<int>(SpriteType::ANIMED)] = born_a;
    sbp[static_cast<int>(SpriteType::NONE)] = born_n;
    smp[static_cast<int>(SpriteType::STATIC)] = movement_s;
    smp[static_cast<int>(SpriteType::ANIMED)] = movement_a;

    for (int i = 0; i < MAX_STATE_NUM - 1; i++)
    {
        born_s[i] = _born_s + i;
        born_a[i] = _born_a + i;
        born_n[i] = _born_n + i;
        movement_s[i] = _movement_s + i;
        movement_a[i] = _movement_a + i;
    }
}

SCSBorn *STGStateMan::MakeChar(const STGCharactorSetting &setting)
{
    SCSMovement *m = nullptr;
    SCSBorn *b = nullptr;

    b = *(sbp[static_cast<int>(setting.Texs.SpriteBornType)] +
          sbn[static_cast<int>(setting.Texs.SpriteBornType)]);
    sbn[static_cast<int>(setting.Texs.SpriteBornType)] =
        (sbn[static_cast<int>(setting.Texs.SpriteBornType)] + 1) % MAX_STATE_NUM;
    b->Init(setting);

    m = *(smp[static_cast<int>(setting.Texs.SpriteMovementType)] +
          smn[static_cast<int>(setting.Texs.SpriteMovementType)]);
    smn[static_cast<int>(setting.Texs.SpriteMovementType)] =
        (smn[static_cast<int>(setting.Texs.SpriteMovementType)] + 1) % MAX_STATE_NUM;
    m->Init(setting);

    b->Next = m;

    return b;
}

SCSBorn *STGStateMan::CopyChar(const SCSBorn *enter, const STGCharactorSetting &setting)
{
    SCSBorn *b;
    SCSMovement *m;

    b = *(sbp[static_cast<int>(setting.Texs.SpriteBornType)] +
          sbn[static_cast<int>(setting.Texs.SpriteBornType)]);
    sbn[static_cast<int>(setting.Texs.SpriteBornType)] =
        (sbn[static_cast<int>(setting.Texs.SpriteBornType)] + 1) % MAX_STATE_NUM;
    b->Copy(enter);

    m = *(smp[static_cast<int>(setting.Texs.SpriteMovementType)] +
          smn[static_cast<int>(setting.Texs.SpriteMovementType)]);
    smn[static_cast<int>(setting.Texs.SpriteMovementType)] =
        (smn[static_cast<int>(setting.Texs.SpriteMovementType)] + 1) % MAX_STATE_NUM;
    m->Copy(enter->Next);

    b->Next = m;

    return b;
}

void STGStateMan::Reset() noexcept
{
    std::memset(sbn, 0, sizeof(sbn));
    std::memset(smn, 0, sizeof(smn));
}