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
#ifdef _DEBUG
    std::cout << "Charactor-" << o.CodeName << "-" << o.ID << " is moving, while " << CodeName << "-" << ID << " is flawed.\n";
#endif

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
#ifdef _DEBUG
    std::cout << "Charactor-" << sc.CodeName << " enable. ID: " << id << ".\n";
#endif

    ID = id;
    Name = sc.Name;
    CodeName = sc.CodeName;
    Physics = body;

    /* Setting userdata. Userdata must update every swap in loop array. */
    body->SetUserData(this);

    /* Prepare the enter state & shooter */
    SPending = enter;
    shooter = sht;
    if (shooter != nullptr)
        speed = shooter->ShiftIn();
    else
        speed = sc.DefaultSpeed;
}

/* This is the final disable. Release the things need to be set free (such as shooter). Pass the point of no return. */
void STGCharactor::Farewell() const noexcept
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " sayonara! ID: " << ID << ".\n";
#endif

    if (shooter != nullptr)
        shooter->ShiftOut(false);
    Con->DisableAll(ID);
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
        if (SNow->CheckInput(&event))
            commands[event.user.data1](this, &event);
#ifdef _DEBUG
        else
            std::cout << "Charactor-" << CodeName << " reject command: " << event.user.data1 << "\n";
#endif

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
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " was hitten, hurt back.\n";
#endif

    o->Hurt(nullptr);
}

void STGCharactor::Hurt(const STGChange *c)
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " was hurt.\n";
#endif

    if (SNow->CheckChange(c, this))
        if (shooter != nullptr)
        {
#ifdef _DEBUG
            std::cout << "Charactor-" << CodeName << " damaged!\n";
#endif

            if (shooter->Hurt(c->any.Damage))
            {
#ifdef _DEBUG
                std::cout << "Charactor-" << CodeName << " lost some shooters!\n";
#endif

                /* Break the chain, disconnected shooter will also lost. Also shift out all followed shooters. */
                int left_chains_l = shooter->Breakchain();

                Shooter *str = shooter;
                while (!str->IsConnected())
                {
                    /* The chain's head is disconnected, shift to next useble one. */
                    str = str->ShiftDown;

                    if (shooter == str)
                    {
#ifdef _DEBUG
                        std::cout << "Charactor-" << CodeName << " lost all shooters!\n";
#endif

                        /* ALL shooter is disconnected. */
                        shooter->ShiftOut(false);
                        shooter = nullptr;
                        return;
                    }
                }

                if (shooter != str)
                {
                    /* Shift the new chain in. */
                    shooter = shooter->ShiftOut();
                    speed = str->ShiftIn();

                    assert(shooter == str);

                    // /* Set new shooter's firing state. */
                    // if (shooter->IsFiring() != str->IsFiring())
                    //     /* The only situation is head shooter is disconnected and firing, while new one is coming. */
                    //     str->SetFire(true);
                    // shooter = str;
                }

                /* Relink the last chain member. */
                if (left_chains_l > 1)
                    str->LinkChain(left_chains_l);
            }
        }
        else
        {
#ifdef _DEBUG
            std::cout << "Charactor-" << CodeName << " go die!\n";
#endif

            /* Hit with no shooter, go die. */
            const STGChange death = {STGStateChangeCode::SSCC_GO_DIE};
            SNow->CheckChange(&death, this);
            ALLEGRO_EVENT event;
            event.user.data1 = STGCharEvent::SCE_GAME_OVER;
            al_emit_user_event(KneeJump, &event, nullptr);
        }
}

/*************************************************************************************************
 *                                                                                               *
 *                             Input Commands Function                                           *
 *                                                                                               *
 *************************************************************************************************/

std::array<std::function<void(STGCharactor *, const ALLEGRO_EVENT *)>, STGCharCommand::SCC_NUM> STGCharactor::commands;

void STGCharactor::InitInputCmd()
{
    commands.fill(std::function<void(STGCharactor *, const ALLEGRO_EVENT *)>(nothing));
    commands[STGCharCommand::SCC_UP] = std::mem_fn(&STGCharactor::up);
    commands[STGCharCommand::SCC_DOWN] = std::mem_fn(&STGCharactor::down);
    commands[STGCharCommand::SCC_LEFT] = std::mem_fn(&STGCharactor::left);
    commands[STGCharCommand::SCC_RIGHT] = std::mem_fn(&STGCharactor::right);
    commands[STGCharCommand::SCC_STG_FIRE] = std::mem_fn(&STGCharactor::shoot);
    commands[STGCharCommand::SCC_STG_CEASE] = std::mem_fn(&STGCharactor::cease);
    commands[STGCharCommand::SCC_STG_CHANGE] = std::mem_fn(&STGCharactor::shift);
    commands[STGCharCommand::SCC_MOVE_XY] = std::mem_fn(&STGCharactor::move_xy);
    commands[STGCharCommand::SCC_STG_CHAIN] = std::mem_fn(&STGCharactor::chain);
    commands[STGCharCommand::SCC_STG_UNCHAIN] = std::mem_fn(&STGCharactor::unchain);
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

void STGCharactor::shoot(const ALLEGRO_EVENT *e) const noexcept
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " shoot!\n";
#endif

    if (shooter != nullptr)
        shooter->SetFire(true);
}

void STGCharactor::cease(const ALLEGRO_EVENT *e) const noexcept
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " stop shooting.\n";
#endif

    if (shooter != nullptr)
        shooter->SetFire(false);
}

void STGCharactor::shift(const ALLEGRO_EVENT *e)
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " shift.\n";
#endif

    if (shooter != nullptr)
    {
        /* Fire state will be auto set by shooters. */
        shooter = shooter->ShiftOut();
        speed = shooter->ShiftIn();
    }
}

void STGCharactor::move_xy(const ALLEGRO_EVENT *e) noexcept
{
    Velocity = *reinterpret_cast<b2Vec2 *>(e->user.data2);
}

void STGCharactor::chain(const ALLEGRO_EVENT *e)
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " chain.\n";
#endif

    if (shooter != nullptr)
        shooter->LinkChain(e->user.data2);
}

void STGCharactor::unchain(const ALLEGRO_EVENT *e)
{
#ifdef _DEBUG
    std::cout << "Charactor-" << CodeName << " unchain.\n";
#endif

    if (shooter != nullptr)
        shooter->Breakchain();
}