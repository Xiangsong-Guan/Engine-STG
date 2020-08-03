#ifndef STG_STATE_H
#define STG_STATE_H

#include "game_event.h"
#include "stg_charactor.h"
#include "anime.h"
#include "cppsuckdef.h"

#include <allegro5/allegro5.h>

constexpr int BORN_TIME = UPDATE_PER_SEC; /* 1 sec. */

enum class STGStateType
{
    BORN,
    MOVEMENT,

    NUM
};

inline void change_texture(const ALLEGRO_BITMAP *t, ALLEGRO_EVENT_SOURCE *s)
{
    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(GameRenderCommand::CHANGE_TEXTURE);
    event.user.data2 = (intptr_t)t;
    al_emit_user_event(s, &event, nullptr);
}

/*************************************************************************************************
 *                                                                                               *
 *                                      Idle / Movement                                          *
 *                                                                                               *
 *************************************************************************************************/

class SCSMovement : public SCS
{
protected:
    int priorty[static_cast<int>(STGCharCommand::NUM)];
    STGCharCommand next;
    int last_time_where;

    inline int move_check(const b2Vec2 v) const noexcept
    {
        int where = static_cast<int>(Movement::IDLE);
        if (v.x > .1f)
            where += static_cast<int>(Movement::TO_RIGHT);
        else if (v.x < -.1f)
            where += static_cast<int>(Movement::TO_LEFT);
        if (v.y > .1f)
            where += static_cast<int>(Movement::TO_DOWN);
        else if (v.y < -.1f)
            where += static_cast<int>(Movement::TO_UP);
        return where;
    }

public:
    SCS *After[static_cast<int>(STGCharCommand::NUM)];

    SCSMovement()
    {
        next = STGCharCommand::STG_CEASE;
        last_time_where = -1;
        priorty[static_cast<int>(STGCharCommand::UP)] = 0;
        priorty[static_cast<int>(STGCharCommand::DOWN)] = 0;
        priorty[static_cast<int>(STGCharCommand::LEFT)] = 0;
        priorty[static_cast<int>(STGCharCommand::RIGHT)] = 0;
        priorty[static_cast<int>(STGCharCommand::STG_FIRE)] = 1;
        priorty[static_cast<int>(STGCharCommand::STG_CEASE)] = 0;
        priorty[static_cast<int>(STGCharCommand::STG_CHANGE)] = 2;
        priorty[static_cast<int>(STGCharCommand::STG_SYNC)] = 3;
        priorty[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_REQUEST)] = 4;
        priorty[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_RESPONE)] = 4;
        priorty[static_cast<int>(STGCharCommand::DISABLE)] = 5;
        priorty[static_cast<int>(STGCharCommand::RESPAWN)] = 5;
        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            After[i] = nullptr;
    }

    virtual bool CheckInput(STGCharCommand cmd) noexcept final
    {
        if (priorty[static_cast<int>(cmd)] > priorty[static_cast<int>(next)])
            next = cmd;
        return true;
    }
};

class SCSMovementStatic : public SCSMovement
{
public:
    ALLEGRO_BITMAP *Texture[static_cast<int>(Movement::NUM)];

    SCSMovementStatic() : SCSMovement() {}
    SCSMovementStatic(ALLEGRO_BITMAP *t[static_cast<int>(Movement::NUM)]) : SCSMovement()
    {
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Texture[i] = t[i];
    }

    virtual void Action(STGCharactor *sc) final
    {
        if (After[static_cast<int>(next)])
        {
            last_time_where = -1;
            After[static_cast<int>(next)]->Action(sc);
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where)
                change_texture(Texture[last_time_where = where], sc->RendererMaster);
        }
    }
};

class SCSMovementAnimed : public SCSMovement
{
public:
    Anime Animation[static_cast<int>(Movement::NUM)];

    SCSMovementAnimed() : SCSMovement() {}
    SCSMovementAnimed(Anime a[static_cast<int>(Movement::NUM)]) : SCSMovement()
    {
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Animation[i] = std::move(a[i]);
    }

    virtual void Action(STGCharactor *sc) final
    {
        if (After[static_cast<int>(next)])
        {
            last_time_where = -1;
            After[static_cast<int>(next)]->Action(sc);
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where)
            {
                Animation[last_time_where].Reset();
                Animation[where].Forward();
                change_texture(Animation[where].Playing, sc->RendererMaster);
                last_time_where = where;
            }
            else if (Animation[where].Forward())
                change_texture(Animation[where].Playing, sc->RendererMaster);
        }
    }
};

/*************************************************************************************************
 *                                                                                               *
 *                                BORN                                                           *
 *                                                                                               *
 *************************************************************************************************/

class SCSBorn : public SCS
{
protected:
    bool filter[static_cast<int>(STGCharCommand::NUM)];

public:
    int Duration;
    int Timer;

    SCSMovement *Next;

    SCSBorn() : Timer(0), Duration(BORN_TIME), Next(nullptr)
    {
        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            filter[i] = false;
        filter[static_cast<int>(STGCharCommand::UP)] = true;
        filter[static_cast<int>(STGCharCommand::DOWN)] = true;
        filter[static_cast<int>(STGCharCommand::LEFT)] = true;
        filter[static_cast<int>(STGCharCommand::RIGHT)] = true;
        filter[static_cast<int>(STGCharCommand::MOVE_XY)] = true;
    }

    virtual bool CheckInput(STGCharCommand cmd) noexcept final
    {
        return filter[static_cast<int>(cmd)];
    }
};

class SCSBornAnimed : public SCSBorn
{
public:
    Anime Animation;

    SCSBornAnimed() : SCSBorn() {}
    SCSBornAnimed(const Anime &a) : SCSBorn(), Animation(a)
    {
        /* Born time should be same with born animation time */
        Duration = Animation.Duration();
    }

    virtual void Action(STGCharactor *sc) final
    {
        Timer += 1;
        if (Timer <= Duration)
        {
            if (Animation.Forward())
                change_texture(Animation.Playing, sc->RendererMaster);
        }
        if (Timer < Duration)
            sc->SPending = this;
        else
        {
            Timer = 0;
            Animation.Reset();
            sc->SPending = Next;
        }
    }
};

class SCSBornNoTexture : public SCSBorn
{
public:
    SCSBornNoTexture() : SCSBorn() {}

    virtual void Action(STGCharactor *sc) final
    {
        Timer += 1;
        if (Timer < Duration)
            sc->SPending = this;
        else
        {
            Timer = 0;
            sc->SPending = Next;
        }
    }
};

class SCSBornStatic : public SCSBorn
{
public:
    ALLEGRO_BITMAP *Texture;

    SCSBornStatic() : SCSBorn() {}
    SCSBornStatic(ALLEGRO_BITMAP *t) : SCSBorn(), Texture(t) {}

    virtual void Action(STGCharactor *sc) final
    {
        Timer += 1;
        if (Timer == Duration)
        {
            Timer = 0;
            sc->SPending = Next;
        }
        if (Timer == 1)
        {
            sc->SPending = this;
            change_texture(Texture, sc->RendererMaster);
        }
    }
};

// class SCSShooting : public SCS
// {
// public:
//     SCSShooting() = default;
//     SCSShooting(const SCSShooting &) = delete;
//     SCSShooting(SCSShooting &&) = delete;
//     SCSShooting &operator=(const SCSShooting &) = delete;
//     SCSShooting &operator=(SCSShooting &&) = delete;
//     ~SCSShooting() = default;

//     virtual void Action(STGCharactor *sc) final;
//     virtual void Enter(STGCharactor *sc) final;
//     virtual void Leave(STGCharactor *sc) final;
// };

// class SCSSync : public SCS
// {
// public:
//     SCSSync() = default;
//     SCSSync(const SCSSync &) = delete;
//     SCSSync(SCSSync &&) = delete;
//     SCSSync &operator=(const SCSSync &) = delete;
//     SCSSync &operator=(SCSSync &&) = delete;
//     ~SCSSync() = default;

//     virtual void Action(STGCharactor *sc) final;
//     virtual void Enter(STGCharactor *sc) final;
//     virtual void Leave(STGCharactor *sc) final;
// };

// class SCSFSync : public SCS
// {
// public:
//     SCSFSync() = default;
//     SCSFSync(const SCSFSync &) = delete;
//     SCSFSync(SCSFSync &&) = delete;
//     SCSFSync &operator=(const SCSFSync &) = delete;
//     SCSFSync &operator=(SCSFSync &&) = delete;
//     ~SCSFSync() = default;

//     virtual void Action(STGCharactor *sc) final;
//     virtual void Enter(STGCharactor *sc) final;
//     virtual void Leave(STGCharactor *sc) final;
// };

// class SCSShift : public SCS
// {
// public:
//     SCSShift() = default;
//     SCSShift(const SCSShift &) = delete;
//     SCSShift(SCSShift &&) = delete;
//     SCSShift &operator=(const SCSShift &) = delete;
//     SCSShift &operator=(SCSShift &&) = delete;
//     ~SCSShift() = default;

//     virtual void Action(STGCharactor *sc) final;
//     virtual void Enter(STGCharactor *sc) final;
//     virtual void Leave(STGCharactor *sc) final;
// };

// class SCSDisbaled : public SCS
// {
// public:
//     SCSDisbaled() = default;
//     SCSDisbaled(const SCSDisbaled &) = delete;
//     SCSDisbaled(SCSDisbaled &&) = delete;
//     SCSDisbaled &operator=(const SCSDisbaled &) = delete;
//     SCSDisbaled &operator=(SCSDisbaled &&) = delete;
//     ~SCSDisbaled() = default;

//     virtual void Action(STGCharactor *sc) final;
//     virtual void Enter(STGCharactor *sc) final;
//     virtual void Leave(STGCharactor *sc) final;
// };

#endif