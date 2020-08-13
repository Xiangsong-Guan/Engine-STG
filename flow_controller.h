#ifndef GAME_FLOW_CONTROLLER_H
#define GAME_FLOW_CONTROLLER_H

#include "data_struct.h"

#include <lua.hpp>

class STGShooter;

class STGFlowController
{
public:
    virtual void Debut(int char_id, float x, float y) = 0;
    virtual void Airborne(int char_id, float x, float y,
                          SCPatternsCode ptn, SCPatternData pd) = 0;
    virtual void Pause() const = 0;
    virtual void DisableAll(int id) = 0;
    virtual void DisableThr(int id) = 0;
    virtual void EnableSht(int id, STGShooter *ss) = 0;
    virtual void DisableSht(int id, STGShooter *ss) = 0;

    virtual const b2Body *TrackEnemy() = 0;
};

class GameFlowController
{
public:
    virtual void Esc() = 0;
    virtual void LinkStart() = 0;
    virtual void LinkEnd() = 0;
    virtual void STGPause() = 0;
    virtual void STGResume() = 0;
    virtual void STGReturn(bool from_level) = 0;
};

#endif