#include "stg_pattern.h"

#include "cppsuckdef.h"
#include "game_event.h"

#include <cmath>

/*************************************************************************************************
 *                                                                                               *
 *                          STGPatthern Initialize / Destroy Function                            *
 *                                                                                               *
 *************************************************************************************************/

STGPattern::STGPattern()
{
    InputMaster = new ALLEGRO_EVENT_SOURCE;
    al_init_user_event_source(InputMaster);
}

STGPattern::~STGPattern()
{
    al_destroy_user_event_source(InputMaster);
    delete InputMaster;
}

void STGPattern::CPPSuckSwap(STGPattern &o) noexcept
{
    std::swap(this->ID, o.ID);
    std::swap(this->InputMaster, o.InputMaster);
    std::swap(this->physics, o.physics);
    std::swap(this->data, o.data);
    std::swap(this->pattern, o.pattern);
}

void STGPattern::Active(int id, SCPatternsCode ptn, SCPatternData pd, b2Body *body) noexcept
{
    pattern = patterns[static_cast<int>(ptn)];
    data = std::move(pd);
    physics = body;
    ID = id;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGPattern::Update()
{
    pattern(this);
}

/*************************************************************************************************
 *                                                                                               *
 *                                 Simple Thinking Patterns                                      *
 *                                                                                               *
 *************************************************************************************************/

std::function<void(STGPattern *)> STGPattern::patterns[static_cast<int>(SCPatternsCode::NUM)];

void STGPattern::InitSTGPattern()
{
    patterns[static_cast<int>(SCPatternsCode::MOVE_LAST)] = std::mem_fn(&STGPattern::move_last);
    patterns[static_cast<int>(SCPatternsCode::MOVE_TO)] = std::mem_fn(&STGPattern::move_to);
    patterns[static_cast<int>(SCPatternsCode::MOVE_PASSBY)] =
        std::mem_fn(&STGPattern::move_passby);
}

void STGPattern::move_to()
{
    b2Vec2 vec = b2Vec2(data.Vec.X, data.Vec.Y) - physics->GetPosition();

    if (vec.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = std::lroundf(vec.x * 1000.f);
        event.user.data3 = std::lroundf(vec.y * 1000.f);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
        Con->DisablePtn(ID);
}

void STGPattern::move_last()
{
    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
    event.user.data2 = std::lroundf(data.Vec.X * 1000.f);
    event.user.data3 = std::lroundf(data.Vec.Y * 1000.f);
    al_emit_user_event(InputMaster, &event, nullptr);
}

void STGPattern::move_passby()
{
    b2Vec2 vec =
        b2Vec2(data.Passby.Vec[data.Passby.Where].X, data.Passby.Vec[data.Passby.Where].Y) -
        physics->GetPosition();

    if (vec.LengthSquared() > physics->GetLinearVelocity().LengthSquared() * SEC_PER_UPDATE_BY2_SQ)
    {
        ALLEGRO_EVENT event;
        event.user.data1 = static_cast<intptr_t>(STGCharCommand::MOVE_XY);
        event.user.data2 = std::lroundf(vec.x * 1000.f);
        event.user.data3 = std::lroundf(vec.y * 1000.f);
        al_emit_user_event(InputMaster, &event, nullptr);
    }
    else
    {
        data.Passby.Where += 1;
        if (data.Passby.Where == data.Passby.Num)
            if (data.Passby.Loop)
                data.Passby.Where = 0;
            else
                Con->DisablePtn(ID);
    }
}