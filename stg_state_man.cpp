#include "stg_state_man.h"

#include "resource_manger.h"

SCS *STGStateMan::MakeChar(const STGCharactorSetting &setting)
{
    SCSMovement *m = nullptr;
    SCSBorn *b = nullptr;

    switch (setting.Texs.SpriteBornType)
    {
    case SpriteType::STATIC:
        b = born_s + num_born_s;
        born_s[num_born_s] =
            SCSBornStatic(ResourceManager::GetTexture(setting.Texs.SpriteBorn));
        num_born_s += 1;
        break;
    case SpriteType::ANIMED:
        b = born_a + num_born_a;
        born_a[num_born_a] =
            SCSBornAnimed(ResourceManager::GetAnime(setting.Texs.SpriteBorn));
        num_born_a += 1;
        break;
    case SpriteType::NONE:
        b = born_n + num_born_n;
        num_born_n += 1;
        break;
    }
    switch (setting.Texs.SpriteMovementType)
    {
    case SpriteType::STATIC:
        m = movement_s + num_movement_s;
        ALLEGRO_BITMAP *t[static_cast<int>(Movement::NUM)];
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            t[i] = ResourceManager::GetTexture(setting.Texs.SpriteMovement[i]);
        movement_s[num_movement_s] = SCSMovementStatic(t);
        num_movement_s += 1;
        break;
    case SpriteType::ANIMED:
        m = movement_a + num_movement_a;
        Anime a[static_cast<int>(Movement::NUM)];
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            a[i] = ResourceManager::GetAnime(setting.Texs.SpriteMovement[i]);
        movement_a[num_movement_a] = SCSMovementAnimed(a);
        num_movement_a += 1;
        break;
    }

    b->Next = m;

    return b;
}

void STGStateMan::Clear() noexcept
{
    num_born_a = 0;
    num_born_s = 0;
    num_born_n = 0;
    num_movement_a = 0;
    num_movement_s = 0;
}