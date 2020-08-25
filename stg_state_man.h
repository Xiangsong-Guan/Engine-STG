#ifndef STG_STATE_MAN_H
#define STG_STATE_MAN_H

#include "data_struct.h"
#include "stg_state.h"
#include "cppsuckdef.h"

#define DEFINE_EVERTHING(scs, scsa, scss, num, p, a, s, n, _a, _s, _n) \
    scs **p[3];                                                        \
    int num[3];                                                        \
    scs *a[MAX_STATE_NUM];                                             \
    scs *s[MAX_STATE_NUM];                                             \
    scs *n[MAX_STATE_NUM];                                             \
    scsa _a[MAX_STATE_NUM];                                            \
    scss _s[MAX_STATE_NUM];                                            \
    scs _n[MAX_STATE_NUM];

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
    SCSBorn *MakeChar(const STGTexture &texs);
    SCSBorn *CopyChar(const SCSBorn *enter, const STGTexture &texs);

    void Reset() noexcept;

private:
    static constexpr int MAX_STATE_NUM = MAX_ENTITIES + MAX_ON_STAGE;

    DEFINE_EVERTHING(SCSBorn, SCSBornAnimed, SCSBornStatic, sbn, sbp, born_a, born_s, born_n, _born_a, _born_s, _born_n);
    DEFINE_EVERTHING(SCSMovement, SCSMovementAnimed, SCSMovementStatic,
                     smn, smp, movement_a, movement_s, movement_n, _movement_a, _movement_s, _movement_n);
    DEFINE_EVERTHING(SCSDisabled, SCSDisabledAnimed, SCSDisabledStatic,
                     sdn, sdp, disabled_a, disabled_s, disabled_n, _disabled_a, _disabled_s, _disabled_n);
};

#endif