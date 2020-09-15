#ifndef STG_STATE_MAN_H
#define STG_STATE_MAN_H

#include "data_struct.h"
#include "stg_state.h"
#include "cppsuckdef.h"

#define DEFINE_EVERTHING(scs, scsa, scss, num, p, a, s, _a, _s) \
    scs **p[SpriteType::SPT_NUM];                               \
    int num[SpriteType::SPT_NUM];                               \
    scs *a[MAX_ENTITIES];                                       \
    scs *s[MAX_ENTITIES];                                       \
    scsa _a[MAX_ENTITIES];                                      \
    scss _s[MAX_ENTITIES];

class STGStateMan
{
public:
    /* Constructor/Destructor */
    STGStateMan();
    STGStateMan(const STGStateMan &) = delete;
    STGStateMan(STGStateMan &&) = delete;
    STGStateMan &operator=(const STGStateMan &) = delete;
    STGStateMan &operator=(STGStateMan &&) = delete;
    ~STGStateMan() = default;

    /* Make Char in STG, Char is so Gan. */
    SCS *MakeChar(const STGTexture &texs);
    SCS *CopyChar(const SCS *enter, const STGTexture &texs);

    void Reset() noexcept;

private:
    SCSBornAnimed born[MAX_ENTITIES];
    int born_n;
    SCSDisabledAnimed disable[MAX_ENTITIES];
    int disable_n;

    DEFINE_EVERTHING(SCSMovement, SCSMovementAnimed, SCSMovementStatic,
                     smn, smp, movement_a, movement_s, _movement_a, _movement_s);

    SCSDisabled one_frame_disable;
};

#endif