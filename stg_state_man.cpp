#include "stg_state_man.h"

#include "resource_manger.h"

#define CONSTRUCT_EVERYTHING(p, s, a, n, _s, _a, _n) \
    {                                                \
        p[static_cast<int>(SpriteType::STATIC)] = s; \
        p[static_cast<int>(SpriteType::ANIMED)] = a; \
        p[static_cast<int>(SpriteType::NONE)] = n;   \
                                                     \
        for (int i = 0; i < MAX_STATE_NUM - 1; i++)  \
        {                                            \
            s[i] = _s + i;                           \
            a[i] = _a + i;                           \
            n[i] = _n + i;                           \
        }                                            \
    }

STGStateMan::STGStateMan()
{
    CONSTRUCT_EVERYTHING(sbp, born_s, born_a, born_n, _born_s, _born_a, _born_n);
    CONSTRUCT_EVERYTHING(smp, movement_s, movement_a, movement_n, _movement_s, _movement_a, _movement_n);
    CONSTRUCT_EVERYTHING(sdp, disabled_s, disabled_a, disabled_n, _disabled_s, _disabled_a, _disabled_n);
}

#define MAKE_EVERYTHING(single, p, n, type)                                \
    {                                                                      \
        single = *(p[static_cast<int>(type)] + n[static_cast<int>(type)]); \
        n[static_cast<int>(type)] += 1;                                    \
        assert(n[static_cast<int>(type)] < MAX_STATE_NUM);                 \
        single->Init(texs);                                                \
    }

SCSBorn *STGStateMan::MakeChar(const STGTexture &texs)
{
    SCSMovement *m = nullptr;
    SCSBorn *b = nullptr;
    SCSDisabled *d = nullptr;

    MAKE_EVERYTHING(b, sbp, sbn, texs.SpriteBornType);
    MAKE_EVERYTHING(m, smp, smn, texs.SpriteMovementType);
    MAKE_EVERYTHING(d, sdp, sdn, texs.SpriteDisableType);

    b->Next = m;
    d->Next = b;
    m->NextDisable = d;

    return b;
}

#define COPY_EVERYTHING(single, p, n, type, source)                        \
    {                                                                      \
        single = *(p[static_cast<int>(type)] + n[static_cast<int>(type)]); \
        n[static_cast<int>(type)] += 1;                                    \
        assert(n[static_cast<int>(type)] < MAX_STATE_NUM);                 \
        single->Copy(source);                                              \
    }

SCSBorn *STGStateMan::CopyChar(const SCSBorn *enter, const STGTexture &texs)
{
    SCSBorn *b = nullptr;
    SCSMovement *m = nullptr;
    SCSDisabled *d = nullptr;

    COPY_EVERYTHING(b, sbp, sbn, texs.SpriteBornType, enter);
    COPY_EVERYTHING(m, smp, smn, texs.SpriteMovementType, enter->Next);
    COPY_EVERYTHING(d, sdp, sdn, texs.SpriteDisableType, enter->Next->NextDisable);

    b->Next = m;
    d->Next = b;
    m->NextDisable = d;

    return b;
}

void STGStateMan::Reset() noexcept
{
    std::memset(sbn, 0, sizeof(sbn));
    std::memset(smn, 0, sizeof(smn));
    std::memset(sdn, 0, sizeof(sdn));
}