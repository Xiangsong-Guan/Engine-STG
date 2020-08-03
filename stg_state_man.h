#ifndef STG_STATE_MAN_H
#define STG_STATE_MAN_H

#include "data_struct.h"
#include "stg_state.h"

class STGStateMan
{
public:
    /* Constructor/Destructor */
    STGStateMan() = default;
    STGStateMan(const STGStateMan &) = delete;
    STGStateMan(STGStateMan &&) = delete;
    STGStateMan &operator=(const STGStateMan &) = delete;
    STGStateMan &operator=(STGStateMan &&) = delete;
    ~STGStateMan() = default;

    /* Make Char in STG, Char is so Gan. */
    SCS *MakeChar(const STGCharactorSetting &setting);

    void Clear() noexcept;

private:
    static constexpr int MAX_STATE_NUM = 2048;

    SCSBornAnimed born_a[MAX_STATE_NUM];
    int num_born_a;
    SCSBornStatic born_s[MAX_STATE_NUM];
    int num_born_s;
    SCSBornNoTexture born_n[MAX_STATE_NUM];
    int num_born_n;

    SCSMovementAnimed movement_a[MAX_STATE_NUM];
    int num_movement_a;
    SCSMovementStatic movement_s[MAX_STATE_NUM];
    int num_movement_s;
};

#endif