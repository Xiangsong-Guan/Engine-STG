#include "stg_state_man.h"

#include "resource_manger.h"

#define CONSTRUCT_EVERYTHING(p, s, a, _s, _a)      \
    {                                              \
        p[SpriteType::SPT_STATIC] = s;             \
        p[SpriteType::SPT_ANIMED] = a;             \
                                                   \
        for (int i = 0; i < MAX_ENTITIES - 1; i++) \
        {                                          \
            s[i] = _s + i;                         \
            a[i] = _a + i;                         \
        }                                          \
    }

STGStateMan::STGStateMan()
{
    CONSTRUCT_EVERYTHING(smp, movement_s, movement_a, _movement_s, _movement_a);
}

#define MAKE_EVERYTHING(single, p, n, type)  \
    {                                        \
        if (type != SpriteType::SPT_NONE)    \
        {                                    \
            single = *(p[type] + n[type]);   \
            n[type] += 1;                    \
            assert(n[type] <= MAX_ENTITIES); \
            single->Init(texs);              \
        }                                    \
    }

SCS *STGStateMan::MakeChar(const STGTexture &texs)
{
    SCS *enter = nullptr;
    SCSMovement *m = nullptr;
    SCSBornAnimed *b = nullptr;
    SCSDisabled *d = nullptr;

    if (texs.SpriteBornType == SpriteType::SPT_ANIMED)
    {
        born[born_n].Init(texs);
        b = born + born_n;
        born_n += 1;

        assert(born_n <= MAX_ENTITIES);
    }

    if (texs.SpriteDisableType == SpriteType::SPT_ANIMED)
    {
        disable[disable_n].Init(texs);
        d = disable + disable_n;
        disable_n += 1;

        assert(disable_n <= MAX_ENTITIES);
    }
    else
        d = &one_frame_disable;

    MAKE_EVERYTHING(m, smp, smn, texs.SpriteMovementType);

    enter = m;
    if (b != nullptr)
    {
        enter = b;
        b->Next = m;
    }
    m->NextDisable = d;

    return enter;
}

#define COPY_EVERYTHING(single, p, n, type, source) \
    {                                               \
        if (type != SpriteType::SPT_NONE)           \
        {                                           \
            single = *(p[type] + n[type]);          \
            n[type] += 1;                           \
            assert(n[type] <= MAX_ENTITIES);        \
            single->Copy(source);                   \
        }                                           \
    }

SCS *STGStateMan::CopyChar(const SCS *enter, const STGTexture &texs)
{
#ifdef _DEBUG
    std::cout << "SCS-" << enter << "copied. Sprite name is"
              << texs.SpriteMovement[Movement::MM_IDLE] << ".\n";
#endif

    SCSBornAnimed *b = nullptr;
    SCSMovement *m = nullptr;
    SCSDisabled *d = nullptr;

    const SCSMovement *movement_enter = nullptr;

    if (texs.SpriteBornType != SpriteType::SPT_ANIMED)
        movement_enter = static_cast<const SCSMovement *>(enter);
    else
    {
        const SCSBornAnimed *born_enter = static_cast<const SCSBornAnimed *>(enter);
        movement_enter = born_enter->Next;

        born[born_n].Copy(born_enter);
        b = born + born_n;
        born_n += 1;

        assert(born_n <= MAX_ENTITIES);
    }

    if (texs.SpriteDisableType == SpriteType::SPT_ANIMED)
    {
        disable[disable_n].Copy(
            reinterpret_cast<SCSDisabledAnimed *>(movement_enter->NextDisable));
        d = disable + disable_n;
        disable_n += 1;

        assert(disable_n <= MAX_ENTITIES);
    }
    else
        d = &one_frame_disable;

    COPY_EVERYTHING(m, smp, smn, texs.SpriteMovementType, movement_enter);

    if (b != nullptr)
        b->Next = m;
    m->NextDisable = d;

    return b == nullptr ? reinterpret_cast<SCS *>(m) : reinterpret_cast<SCS *>(b);
}

void STGStateMan::Reset() noexcept
{
    born_n = 0;
    disable_n = 0;
    std::memset(smn, 0, sizeof(smn));
}