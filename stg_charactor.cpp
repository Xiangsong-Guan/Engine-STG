#include "stg_charactor.h"

#include "resource_manger.h"

#include <iostream>

static inline void nothing(STGCharactor *sc, const ALLEGRO_EVENT *e) noexcept {}

/*************************************************************************************************
 *                                                                                               *
 *                                Initialize / Destroy Function                                  *
 *                                                                                               *
 *************************************************************************************************/

STGCharactor::STGCharactor()
{
    InputTerminal = al_create_event_queue();
    if (!InputTerminal)
    {
        std::cerr << "Failed to initialize STG charactors' event queue!\n";
        std::abort();
    }

    RendererMaster = new ALLEGRO_EVENT_SOURCE;
    al_init_user_event_source(RendererMaster);
    KneeJump = new ALLEGRO_EVENT_SOURCE;
    al_init_user_event_source(KneeJump);
}

STGCharactor::~STGCharactor()
{
    al_destroy_event_queue(InputTerminal);

    al_destroy_user_event_source(RendererMaster);
    al_destroy_user_event_source(KneeJump);
    delete KneeJump;
    delete RendererMaster;
}

void STGCharactor::CPPSuckSwap(STGCharactor &o) noexcept
{
    std::swap(this->ID, o.ID);
    std::swap(this->KneeJump, o.KneeJump);
    std::swap(this->RendererMaster, o.RendererMaster);
    std::swap(this->InputTerminal, o.InputTerminal);
    std::swap(this->Physics, o.Physics);
    std::swap(this->SNow, o.SNow);
    std::swap(this->SPending, o.SPending);
    std::swap(this->speed, o.speed);
}

/* Do something when char is loaded. */
void STGCharactor::Enable(int id, b2Body *body, const STGCharactorSetting &setting, SCS *enter)
{
    ID = id;

    /* Attach fixture, so char can be collsion. */
    Physics = body;
    if (setting.Phy.Physical)
        Physics->CreateFixture(&setting.Phy.FD);

    /* Setting userdata. Userdata must update every swap in loop array. */
    body->SetUserData(this);

    /* Prepare the enter state */
    SPending = enter;

    speed = setting.DefaultSpeed;
}

/*************************************************************************************************
 *                                                                                               *
 *                                  Update    Function                                           *
 *                                                                                               *
 *************************************************************************************************/

void STGCharactor::Update()
{
    SNow = SPending;
    Velocity.SetZero();

    ALLEGRO_EVENT event;
    while (al_get_next_event(InputTerminal, &event))
        if (SNow->CheckInput(static_cast<STGCharCommand>(event.user.data1)))
            commands[event.user.data1](this, &event);

    Velocity.Normalize();
    Velocity *= speed;

    /* update state (enter new state / fresh animation / block some intput / update status) */
    SNow->Action(this);

    Physics->SetLinearVelocity(Velocity);
    Physics->SetUserData(this);
}

/*************************************************************************************************
 *                                                                                               *
 *                             Input Commands Function                                           *
 *                                                                                               *
 *************************************************************************************************/

std::array<std::function<void(STGCharactor *, const ALLEGRO_EVENT *)>,
           static_cast<int>(STGCharCommand::NUM)>
    STGCharactor::commands;

void STGCharactor::InitInputCmd()
{
    commands.fill(std::function<void(STGCharactor *, const ALLEGRO_EVENT *)>(nothing));
    commands[static_cast<int>(STGCharCommand::UP)] = std::mem_fn(&STGCharactor::up);
    commands[static_cast<int>(STGCharCommand::DOWN)] = std::mem_fn(&STGCharactor::down);
    commands[static_cast<int>(STGCharCommand::LEFT)] = std::mem_fn(&STGCharactor::left);
    commands[static_cast<int>(STGCharCommand::RIGHT)] = std::mem_fn(&STGCharactor::right);
    commands[static_cast<int>(STGCharCommand::STG_FIRE)] = std::mem_fn(&STGCharactor::shoot);
    commands[static_cast<int>(STGCharCommand::STG_CEASE)] = std::mem_fn(&STGCharactor::cease);
    commands[static_cast<int>(STGCharCommand::STG_CHANGE)] = std::mem_fn(&STGCharactor::shift);
    commands[static_cast<int>(STGCharCommand::STG_SYNC)] = std::mem_fn(&STGCharactor::sync);
    commands[static_cast<int>(STGCharCommand::MOVE_XY)] = std::mem_fn(&STGCharactor::move_xy);
}

void STGCharactor::up(const ALLEGRO_EVENT *e) noexcept
{
    Velocity.y -= 1.f;
}

void STGCharactor::down(const ALLEGRO_EVENT *e) noexcept
{
    Velocity.y += 1.f;
}

void STGCharactor::left(const ALLEGRO_EVENT *e) noexcept
{
    Velocity.x -= 1.f;
}

void STGCharactor::right(const ALLEGRO_EVENT *e) noexcept
{
    Velocity.x += 1.f;
}

void STGCharactor::shoot(const ALLEGRO_EVENT *e)
{
    puts("Player shooting!");
}

void STGCharactor::cease(const ALLEGRO_EVENT *e)
{
    puts("Player cease!!");
}

void STGCharactor::shift(const ALLEGRO_EVENT *e)
{
    puts("Player shift!!");
}

void STGCharactor::sync(const ALLEGRO_EVENT *e)
{
    puts("Player sync!!");
}

void STGCharactor::move_xy(const ALLEGRO_EVENT *e) noexcept
{
    Velocity.x = static_cast<float>(e->user.data2);
    Velocity.y = static_cast<float>(e->user.data3);
}