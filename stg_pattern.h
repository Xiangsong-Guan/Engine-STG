#ifndef STG_PATTERN_H
#define STG_PATTERN_H

#include "data_struct.h"
#include "flow_controller.h"

#include <allegro5/allegro5.h>
#include <box2d/box2d.h>

#include <functional>

class SCPattern
{
public:
    int ID;

    static void InitSCPattern();

    SCPattern();
    SCPattern(const SCPattern &) = delete;
    SCPattern(SCPattern &&) = delete;
    SCPattern &operator=(const SCPattern &) = delete;
    SCPattern &operator=(SCPattern &&) = delete;
    ~SCPattern();
    void CPPSuckSwap(SCPattern &) noexcept;

    void Active(int id, SCPatternsCode ptn, SCPatternData pd, b2Body *body) noexcept;
    void Update();

    ALLEGRO_EVENT_SOURCE *InputMaster;

    STGFlowController *Con;

private:
    b2Body *physics;
    SCPatternData data;
    std::function<void(SCPattern *)> pattern;

    static std::function<void(SCPattern *)> patterns[static_cast<int>(SCPatternsCode::NUM)];
    void move_to();
    void move_last();
    void move_passby();
};

#endif