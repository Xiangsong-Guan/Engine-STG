#ifndef STG_STATE_MAN_H
#define STG_STATE_MAN_H

#include "data_struct.h"
#include "stg_state.h"
#include "cppsuckdef.h"

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
    SCSBorn *MakeChar(const STGCharactorSetting &setting);
    SCSBorn *CopyChar(const SCSBorn *enter, const STGCharactorSetting &setting);

    void Reset() noexcept;

private:
    static constexpr int MAX_STATE_NUM = MAX_ENTITIES + MAX_ON_STAGE;

    SCSBorn **sbp[3];
    int sbn[3];
    SCSBorn *born_a[MAX_STATE_NUM];
    SCSBorn *born_s[MAX_STATE_NUM];
    SCSBorn *born_n[MAX_STATE_NUM];
    SCSBornAnimed _born_a[MAX_STATE_NUM];
    SCSBornStatic _born_s[MAX_STATE_NUM];
    SCSBornNoTexture _born_n[MAX_STATE_NUM];

    SCSMovement **smp[2];
    int smn[2];
    SCSMovement *movement_a[MAX_STATE_NUM];
    SCSMovement *movement_s[MAX_STATE_NUM];
    SCSMovementAnimed _movement_a[MAX_STATE_NUM];
    SCSMovementStatic _movement_s[MAX_STATE_NUM];
};

#endif