#ifndef STG_STATE_H
#define STG_STATE_H

#include "game_event.h"
#include "stg_charactor.h"
#include "anime.h"
#include "cppsuckdef.h"
#include "data_struct.h"
#include "resource_manger.h"

#include <allegro5/allegro5.h>

constexpr int BORN_TIME = UPDATE_PER_SEC; /* 1 sec. */

static inline void change_texture(const ALLEGRO_BITMAP *t, ALLEGRO_EVENT_SOURCE *s)
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
    int PRIORTY[static_cast<int>(STGCharCommand::NUM)];
    STGCharCommand next_i;
    int last_time_where;
    bool full;

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

    inline void init(bool no_shooter) noexcept
    {
        next_i = STGCharCommand::UP;
        last_time_where = -1;

        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            Next[i] = nullptr;

        if (no_shooter)
        {
            PRIORTY[static_cast<int>(STGCharCommand::STG_FIRE)] = -1;
            PRIORTY[static_cast<int>(STGCharCommand::STG_CEASE)] = -1;
        }
    }

public:
    SCS *Next[static_cast<int>(STGCharCommand::NUM)];

    SCSMovement()
    {
        PRIORTY[static_cast<int>(STGCharCommand::UP)] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::DOWN)] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::LEFT)] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::RIGHT)] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::STG_FIRE)] = 1;
        PRIORTY[static_cast<int>(STGCharCommand::STG_CEASE)] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::STG_CHANGE)] = 2;
        PRIORTY[static_cast<int>(STGCharCommand::STG_SYNC)] = 3;
        PRIORTY[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_REQUEST)] = 4;
        PRIORTY[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_RESPONE)] = 4;
        PRIORTY[static_cast<int>(STGCharCommand::DISABLE)] = 5;
        PRIORTY[static_cast<int>(STGCharCommand::RESPAWN)] = 5;
    }
    SCSMovement(const SCSMovement &) = delete;
    SCSMovement(SCSMovement &&) = delete;
    SCSMovement &operator=(const SCSMovement &) = delete;
    SCSMovement &operator=(SCSMovement &&) = delete;
    virtual ~SCSMovement() = default;

    bool CheckInput(STGCharCommand cmd) noexcept final
    {
        if (PRIORTY[static_cast<int>(cmd)] > PRIORTY[static_cast<int>(next_i)])
            next_i = cmd;
        return PRIORTY[static_cast<int>(cmd)] >= 0;
    }

    virtual void Init(const STGTexture &texs, bool no_shooter)
    {
        init(no_shooter);
    }

    virtual void Copy(const SCSMovement *o, bool no_shooter)
    {
        init(no_shooter);
    }

    virtual void Action(STGCharactor *sc) override
    {
        if (Next[static_cast<int>(next_i)])
            Next[static_cast<int>(next_i)]->Action(sc);
        else
            sc->SPending = this;
    }

    inline bool IsFull() const noexcept
    {
        return full;
    }
};

class SCSMovementStatic : public SCSMovement
{
public:
    ALLEGRO_BITMAP *Texture[static_cast<int>(Movement::NUM)];

    SCSMovementStatic() = default;
    SCSMovementStatic(const SCSMovementStatic &) = delete;
    SCSMovementStatic(SCSMovementStatic &&) = delete;
    SCSMovementStatic &operator=(const SCSMovementStatic &) = delete;
    SCSMovementStatic &operator=(SCSMovementStatic &&) = delete;
    ~SCSMovementStatic() = default;

    void Init(const STGTexture &texs, bool no_shooter) final
    {
        init(no_shooter);
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Texture[i] = ResourceManager::GetTexture(texs.SpriteMovement[i]);
        full = texs.FullMovementSprites;
    }

    void Copy(const SCSMovement *o, bool no_shooter) final
    {
        init(no_shooter);
        std::memcpy(Texture, dynamic_cast<const SCSMovementStatic *>(o)->Texture, sizeof(Texture));
        full = o->IsFull();
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[static_cast<int>(next_i)])
        {
            last_time_where = -1;
            Next[static_cast<int>(next_i)]->Action(sc);
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where && full)
                change_texture(Texture[last_time_where = where], sc->RendererMaster);
        }
    }
};

class SCSMovementAnimed : public SCSMovement
{
public:
    Anime Animation[static_cast<int>(Movement::NUM)];

    SCSMovementAnimed() = default;
    SCSMovementAnimed(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed(SCSMovementAnimed &&) = delete;
    SCSMovementAnimed &operator=(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed &operator=(SCSMovementAnimed &&) = delete;
    ~SCSMovementAnimed() = default;

    void Init(const STGTexture &texs, bool no_shooter) final
    {
        init(no_shooter);
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Animation[i] = ResourceManager::GetAnime(texs.SpriteMovement[i]);
        full = texs.FullMovementSprites;
    }

    void Copy(const SCSMovement *o, bool no_shooter) final
    {
        init(no_shooter);
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
        {
            Animation[i] = dynamic_cast<const SCSMovementAnimed *>(o)->Animation[i];
            Animation[i].Reset();
        }
        full = o->IsFull();
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[static_cast<int>(next_i)])
        {
            last_time_where = -1;
            Next[static_cast<int>(next_i)]->Action(sc);
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where && full)
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
    bool FILTER[static_cast<int>(STGCharCommand::NUM)];

    inline void init()
    {
        Duration = BORN_TIME;
        Next = nullptr;
        Timer = 0;
    }

public:
    int Duration;
    int Timer;

    SCSMovement *Next;

    SCSBorn()
    {
        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            FILTER[i] = false;
        FILTER[static_cast<int>(STGCharCommand::UP)] = true;
        FILTER[static_cast<int>(STGCharCommand::DOWN)] = true;
        FILTER[static_cast<int>(STGCharCommand::LEFT)] = true;
        FILTER[static_cast<int>(STGCharCommand::RIGHT)] = true;
        FILTER[static_cast<int>(STGCharCommand::MOVE_XY)] = true;
    }
    SCSBorn(const SCSBorn &) = delete;
    SCSBorn(SCSBorn &&) = delete;
    SCSBorn &operator=(const SCSBorn &) = delete;
    SCSBorn &operator=(SCSBorn &&) = delete;
    virtual ~SCSBorn() = default;

    bool CheckInput(STGCharCommand cmd) noexcept final
    {
        return FILTER[static_cast<int>(cmd)];
    }

    virtual void Init(const STGTexture &texs)
    {
        init();
    }

    virtual void Copy(const SCSBorn *o)
    {
        init();
    }

    virtual void Action(STGCharactor *sc) override
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

class SCSBornAnimed : public SCSBorn
{
public:
    Anime Animation;

    SCSBornAnimed() = default;
    SCSBornAnimed(const SCSBornAnimed &) = delete;
    SCSBornAnimed(SCSBornAnimed &&) = delete;
    SCSBornAnimed &operator=(const SCSBornAnimed &) = delete;
    SCSBornAnimed &operator=(SCSBornAnimed &&) = delete;
    ~SCSBornAnimed() = default;

    void Init(const STGTexture &texs) final
    {
        init();
        Animation = ResourceManager::GetAnime(texs.SpriteBorn);
        /* Born time should be same with born animation time */
        Duration = Animation.LG_DURATION;
    }

    void Copy(const SCSBorn *o) final
    {
        init();
        Animation = dynamic_cast<const SCSBornAnimed *>(o)->Animation;
        Animation.Reset();
        /* Born time should be same with born animation time */
        Duration = Animation.LG_DURATION;
    }

    void Action(STGCharactor *sc) final
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

class SCSBornStatic : public SCSBorn
{
public:
    ALLEGRO_BITMAP *Texture;

    SCSBornStatic() = default;
    SCSBornStatic(const SCSBornStatic &) = delete;
    SCSBornStatic(SCSBornStatic &&) = delete;
    SCSBornStatic &operator=(const SCSBornStatic &) = delete;
    SCSBornStatic &operator=(SCSBornStatic &&) = delete;
    ~SCSBornStatic() = default;

    void Init(const STGTexture &texs) final
    {
        Texture = ResourceManager::GetTexture(texs.SpriteBorn);
        init();
    }

    void Copy(const SCSBorn *o) final
    {
        Texture = dynamic_cast<const SCSBornStatic *>(o)->Texture;
        init();
    }

    void Action(STGCharactor *sc) final
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

//     void Action(STGCharactor *sc) final;
//     void Enter(STGCharactor *sc) final;
//     void Leave(STGCharactor *sc) final;
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

//     void Action(STGCharactor *sc) final;
//     void Enter(STGCharactor *sc) final;
//     void Leave(STGCharactor *sc) final;
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

//     void Action(STGCharactor *sc) final;
//     void Enter(STGCharactor *sc) final;
//     void Leave(STGCharactor *sc) final;
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

//     void Action(STGCharactor *sc) final;
//     void Enter(STGCharactor *sc) final;
//     void Leave(STGCharactor *sc) final;
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

//     void Action(STGCharactor *sc) final;
//     void Enter(STGCharactor *sc) final;
//     void Leave(STGCharactor *sc) final;
// };

#endif