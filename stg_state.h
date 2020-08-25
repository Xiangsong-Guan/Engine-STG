#ifndef STG_STATE_H
#define STG_STATE_H

#include "game_event.h"
#include "stg_charactor.h"
#include "anime.h"
#include "cppsuckdef.h"
#include "data_struct.h"
#include "resource_manger.h"

#include <allegro5/allegro5.h>

constexpr int BORN_TIME = UPDATE_PER_SEC / 2; /* .5 sec. */
constexpr int DEAD_TIME = 3;                  /* Juesi. */

static inline void change_texture(const ALLEGRO_BITMAP *t, ALLEGRO_EVENT_SOURCE *s)
{
    ALLEGRO_EVENT event;
    event.user.data1 = static_cast<intptr_t>(GameRenderCommand::CHANGE_TEXTURE);
    event.user.data2 = (intptr_t)t;
    al_emit_user_event(s, &event, nullptr);
}

class SCSMovement;
class SCSBorn;
class SCSDisabled;

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
        /* When use pending to change, make change happens in advance one frame. */
        Duration = BORN_TIME - 1;
        Next = nullptr;
        Timer = -1;
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

    bool CheckInput(STGCharCommand cmd) noexcept final { return FILTER[static_cast<int>(cmd)]; }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final { return false; }

    virtual void Init(const STGTexture &texs) { init(); }

    virtual void Copy(const SCSBorn *o) { init(); }

    virtual void Action(STGCharactor *sc) override
    {
        Timer += 1;
        if (Timer == 0)
            sc->SPending = this;
        else if (Timer == Duration)
        {
            Timer = -1;
            sc->SPending = reinterpret_cast<SCS *>(Next);
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
        Duration = Animation.LG_DURATION - 1;
    }

    void Copy(const SCSBorn *o) final
    {
        init();
        Animation = dynamic_cast<const SCSBornAnimed *>(o)->Animation;
        Animation.Reset();
        /* Born time should be same with born animation time */
        Duration = Animation.LG_DURATION - 1;
    }

    void Action(STGCharactor *sc) final
    {
        Timer += 1;
        /* Last animation frame (in logic) must return false. */
        if (Timer < Duration)
        {
            sc->SPending = this;
            if (Animation.Forward())
                change_texture(Animation.Playing, sc->RendererMaster);
        }
        else
        {
            Timer = -1;
            Animation.Reset();
            sc->SPending = reinterpret_cast<SCS *>(Next);
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
            Timer = -1;
            sc->SPending = reinterpret_cast<SCS *>(Next);
        }
        else if (Timer == 0)
        {
            sc->SPending = this;
            change_texture(Texture, sc->RendererMaster);
        }
    }
};

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

/*************************************************************************************************
 *                                                                                               *
 *                                          Disable                                              *
 *                                                                                               *
 *************************************************************************************************/

class SCSDisabled : public SCS
{
protected:
    bool tomaranaizoooooo;
    int timer;

public:
    int Duration;
    SCSBorn *Next;

    SCSDisabled() = default;
    SCSDisabled(const SCSDisabled &) = delete;
    SCSDisabled(SCSDisabled &&) = delete;
    SCSDisabled &operator=(const SCSDisabled &) = delete;
    SCSDisabled &operator=(SCSDisabled &&) = delete;
    virtual ~SCSDisabled() = default;

    inline void init() noexcept
    {
        timer = -1;
        tomaranaizoooooo = false;
        Duration = DEAD_TIME - 1;
    }

    virtual void Init(const STGTexture &texs) { init(); }

    virtual void Copy(const SCSDisabled *o) { init(); }

    virtual void Action(STGCharactor *sc)
    {
        timer += 1;
        if (timer == 0)
        {
            sc->SPending = this;
            sc->SNow = this;
        }
        else if (timer == 1)
            sc->Physics->SetActive(false);
        else if (timer == Duration)
            if (tomaranaizoooooo)
            {
                timer = -1;
                sc->SPending = Next;
                sc->Physics->SetActive(true);
            }
            else
                sc->Con->DisableAll(sc->ID);
    }

    bool CheckInput(STGCharCommand cmd) final
    {
        if (cmd != STGCharCommand::RESPAWN)
            return false;
        else
            tomaranaizoooooo = true;
        return true;
    }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final { return false; }
};

class SCSDisabledAnimed : public SCSDisabled
{
public:
    Anime Animation;

    SCSDisabledAnimed() = default;
    SCSDisabledAnimed(const SCSDisabledAnimed &) = delete;
    SCSDisabledAnimed(SCSDisabledAnimed &&) = delete;
    SCSDisabledAnimed &operator=(const SCSDisabledAnimed &) = delete;
    SCSDisabledAnimed &operator=(SCSDisabledAnimed &&) = delete;
    ~SCSDisabledAnimed() = default;

    virtual void Init(const STGTexture &texs) final
    {
        init();
        Animation = ResourceManager::GetAnime(texs.SpriteDisable);
        Duration = Animation.LG_DURATION;
    }

    virtual void Copy(const SCSDisabled *o) final
    {
        init();
        Animation = dynamic_cast<const SCSDisabledAnimed *>(o)->Animation;
        Duration = Animation.LG_DURATION;
    }

    virtual void Action(STGCharactor *sc) final
    {
        timer += 1;
        if (timer == 0)
        {
            sc->SPending = this;
            sc->SNow = this;
        }
        else if (timer == 1)
            sc->Physics->SetActive(false);

        if (timer < Duration)
        {
            if (Animation.Forward())
                change_texture(Animation.Playing, sc->RendererMaster);
        }
        else
        {
            if (tomaranaizoooooo)
            {
                timer = -1;
                Animation.Reset();
                sc->SPending = Next;
                sc->Physics->SetActive(true);
            }
            else
                sc->Con->DisableAll(sc->ID);
        }
    }
};

class SCSDisabledStatic : public SCSDisabled
{
public:
    ALLEGRO_BITMAP *Texture;

    SCSDisabledStatic() = default;
    SCSDisabledStatic(const SCSDisabledStatic &) = delete;
    SCSDisabledStatic(SCSDisabledStatic &&) = delete;
    SCSDisabledStatic &operator=(const SCSDisabledStatic &) = delete;
    SCSDisabledStatic &operator=(SCSDisabledStatic &&) = delete;
    ~SCSDisabledStatic() = default;

    virtual void Init(const STGTexture &texs) final
    {
        init();
        Texture = ResourceManager::GetTexture(texs.SpriteDisable);
    }

    virtual void Copy(const SCSDisabled *o) final
    {
        init();
        Texture = dynamic_cast<const SCSDisabledStatic *>(o)->Texture;
    }

    virtual void Action(STGCharactor *sc) final
    {
        timer += 1;
        if (timer == 0)
        {
            sc->SPending = this;
            sc->SNow = this;
            change_texture(Texture, sc->RendererMaster);
        }
        else if (timer == 1)
            sc->Physics->SetActive(false);
        else if (timer == Duration)
            if (tomaranaizoooooo)
            {
                timer = -1;
                sc->SPending = Next;
                sc->Physics->SetActive(true);
            }
            else
                sc->Con->DisableAll(sc->ID);
    }
};

/*************************************************************************************************
 *                                                                                               *
 *                                      Idle / Movement                                          *
 *                                                                                               *
 *************************************************************************************************/

class SCSMovement : public SCS
{
protected:
    int PRIORTY[static_cast<int>(STGCharCommand::NUM)];
    int next_i;
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

    inline void init() noexcept
    {
        next_i = static_cast<int>(STGCharCommand::UP);
        last_time_where = -1;

        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            Next[i] = nullptr;
        NextDisable = nullptr;
    }

public:
    SCS *Next[static_cast<int>(STGCharCommand::NUM)];
    SCSDisabled *NextDisable;

    SCSMovement()
    {
        for (int i = 0; i < static_cast<int>(STGCharCommand::NUM); i++)
            PRIORTY[i] = 0;
        PRIORTY[static_cast<int>(STGCharCommand::STG_CHANGE)] = 9;
        PRIORTY[static_cast<int>(STGCharCommand::STG_SYNC)] = 9;
        PRIORTY[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_REQUEST)] = 10;
        PRIORTY[static_cast<int>(STGCharCommand::STG_FORCE_SYNC_RESPONE)] = 10;
        PRIORTY[static_cast<int>(STGCharCommand::DISABLE)] = 999;
    }
    SCSMovement(const SCSMovement &) = delete;
    SCSMovement(SCSMovement &&) = delete;
    SCSMovement &operator=(const SCSMovement &) = delete;
    SCSMovement &operator=(SCSMovement &&) = delete;
    virtual ~SCSMovement() = default;

    bool CheckInput(STGCharCommand cmd) noexcept final
    {
        if (PRIORTY[static_cast<int>(cmd)] > PRIORTY[next_i])
            next_i = static_cast<int>(cmd);
        return PRIORTY[static_cast<int>(cmd)] >= 0;
    }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final
    {
        if (change->Code == STGStateChangeCode::GO_DIE)
            NextDisable->Action(sc);
        return true;
    }

    virtual void Init(const STGTexture &texs) { init(); }

    virtual void Copy(const SCSMovement *o) { init(); }

    virtual void Action(STGCharactor *sc) override
    {
        if (Next[static_cast<int>(next_i)])
        {
            next_i = static_cast<int>(STGCharCommand::UP);
            Next[static_cast<int>(next_i)]->Action(sc);
        }
        else
            sc->SPending = this;
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

    void Init(const STGTexture &texs) final
    {
        init();
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Texture[i] = ResourceManager::GetTexture(texs.SpriteMovement[i]);
    }

    void Copy(const SCSMovement *o) final
    {
        init();
        std::memcpy(Texture, dynamic_cast<const SCSMovementStatic *>(o)->Texture, sizeof(Texture));
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[static_cast<int>(next_i)])
        {
            next_i = static_cast<int>(STGCharCommand::UP);
            last_time_where = -1;
            Next[static_cast<int>(next_i)]->Action(sc);
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

    SCSMovementAnimed() = default;
    SCSMovementAnimed(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed(SCSMovementAnimed &&) = delete;
    SCSMovementAnimed &operator=(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed &operator=(SCSMovementAnimed &&) = delete;
    ~SCSMovementAnimed() = default;

    void Init(const STGTexture &texs) final
    {
        init();
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
            Animation[i] = ResourceManager::GetAnime(texs.SpriteMovement[i]);
    }

    void Copy(const SCSMovement *o) final
    {
        init();
        for (int i = 0; i < static_cast<int>(Movement::NUM); i++)
        {
            Animation[i] = dynamic_cast<const SCSMovementAnimed *>(o)->Animation[i];
            Animation[i].Reset();
        }
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[static_cast<int>(next_i)])
        {
            next_i = static_cast<int>(STGCharCommand::UP);
            last_time_where = -1;
            Next[static_cast<int>(next_i)]->Action(sc);
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where)
            {
                Animation[last_time_where = where].Reset();
                Animation[where].Forward();
                change_texture(Animation[where].Playing, sc->RendererMaster);
            }
            else if (Animation[where].Forward())
                change_texture(Animation[where].Playing, sc->RendererMaster);
        }
    }
};

#endif