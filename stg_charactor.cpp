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
    this->ID = o.ID;
    this->Name = o.Name;
    this->CodeName = o.CodeName;
    this->Physics = o.Physics;
    this->SNow = o.SNow;
    this->SPending = o.SPending;
    this->speed = o.speed;
    this->shooter = o.shooter;
    std::swap(this->KneeJump, o.KneeJump);
    std::swap(this->RendererMaster, o.RendererMaster);
    std::swap(this->InputTerminal, o.InputTerminal);
}

/* Do something when char is loaded. */
void STGCharactor::Enable(int id, const STGCharactorSetting &sc, b2Body *body, Shooter *sht, SCS *enter)
{
    ID = id;
    Name = sc.Name;
    CodeName = sc.CodeName;
    Physics = body;

    /* Setting userdata. Userdata must update every swap in loop array. */
    body->SetUserData(this);

    /* Prepare the enter state & shooter */
    speed = sc.DefaultSpeed;
    SPending = enter;
    shooter = sht;
    if (shooter != nullptr)
        speed = shooter->ShiftIn(false);
}

/* This is the final disable. Release the things need to be set free (such as shooter). Pass the point of no return. */
void STGCharactor::Farewell() const noexcept
{
    shooter->ShiftOut();
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
 *                                      Collision Handle                                         *
 *                                                                                               *
 *************************************************************************************************/

void STGCharactor::Hit(CollisionHandler *o)
{
    o->Hurt(nullptr);
}

void STGCharactor::Hurt(const STGChange *c)
{
    if (SNow->CheckChange(c, this) && shooter != nullptr)
    {
#ifdef _DEBUG
        std::cout << "Char-" << ID << " hurt!\n";
#endif
        if (shooter->Hurt(c->any.Damage))
        {
#ifdef _DEBUG
            std::cout << "Char-" << ID << " go die!\n";
#endif

            const STGChange death = {STGStateChangeCode::GO_DIE};
            if (SNow->CheckChange(&death, this))
            {
                /* Disable all charactor's effect. */
                // ...
            }
            ALLEGRO_EVENT event;
            event.user.data1 = static_cast<int>(STGCharEvent::GAME_OVER);
            al_emit_user_event(KneeJump, &event, nullptr);
        }
    }
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
    if (shooter != nullptr)
        shooter->Fire();
}

void STGCharactor::cease(const ALLEGRO_EVENT *e)
{
    if (shooter != nullptr)
        shooter->Cease();
}

void STGCharactor::shift(const ALLEGRO_EVENT *e)
{
    if (shooter != nullptr)
    {
        bool firing = shooter->ShiftOut();

        shooter = shooter->Shift;
        speed = shooter->ShiftIn(firing);
    }
}

void STGCharactor::sync(const ALLEGRO_EVENT *e)
{
    puts("Player sync!!");
}

void STGCharactor::move_xy(const ALLEGRO_EVENT *e) noexcept
{
    Velocity = *reinterpret_cast<b2Vec2 *>(e->user.data2);
}