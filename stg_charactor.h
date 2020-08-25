#ifndef STG_CHARACTOR_H
#define STG_CHARACTOR_H

#include "data_struct.h"
#include "game_event.h"
#include "flow_controller.h"
#include "shooter.h"
#include "contact_listener.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <functional>
#include <array>

class STGCharactor;

class SCS
{
public:
    SCS() = default;
    SCS(const SCS &) = delete;
    SCS(SCS &&) = delete;
    SCS &operator=(const SCS &) = delete;
    SCS &operator=(SCS &&) = delete;
    virtual ~SCS() = default;

    virtual void Action(STGCharactor *sc) = 0;
    virtual bool CheckInput(STGCharCommand cmd) = 0;
    virtual bool CheckChange(const STGChange *change) = 0;
};

class STGCharactor : public CollisionHandler
{
public:
    int ID;
    std::string Name;
    std::string CodeName;

    STGFlowController *Con;

    ALLEGRO_EVENT_SOURCE *KneeJump;
    ALLEGRO_EVENT_SOURCE *RendererMaster;
    ALLEGRO_EVENT_QUEUE *InputTerminal;

    b2Vec2 Velocity;
    b2Body *Physics;

    SCS *SNow, *SPending;

    STGCharactor();
    STGCharactor(const STGCharactor &) = delete;
    STGCharactor(STGCharactor &&) = delete;
    STGCharactor &operator=(const STGCharactor &) = delete;
    STGCharactor &operator=(STGCharactor &&) = delete;
    ~STGCharactor();
    void CPPSuckSwap(STGCharactor &) noexcept;

    void Enable(int id, const STGCharactorSetting &sc, b2Body *body, Shooter *sht, SCS *enter);
    void Update();

    static void InitInputCmd();

    void Hit(CollisionHandler *o) final;
    void Hurt(const STGChange *c) final;

private:
    float speed;
    Shooter *shooter;

    static std::array<std::function<void(STGCharactor *, const ALLEGRO_EVENT *)>,
                      static_cast<int>(STGCharCommand::NUM)>
        commands;

    void up(const ALLEGRO_EVENT *e) noexcept;
    void down(const ALLEGRO_EVENT *e) noexcept;
    void left(const ALLEGRO_EVENT *e) noexcept;
    void right(const ALLEGRO_EVENT *e) noexcept;
    void shoot(const ALLEGRO_EVENT *e);
    void cease(const ALLEGRO_EVENT *e);
    void shift(const ALLEGRO_EVENT *e);
    void sync(const ALLEGRO_EVENT *e);
    void move_xy(const ALLEGRO_EVENT *e) noexcept;
};

#endif