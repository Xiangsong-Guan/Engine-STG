#ifndef STG_STATE_H
#define STG_STATE_H

#include "game_event.h"
#include "stg_charactor.h"
#include "anime.h"
#include "cppsuckdef.h"
#include "data_struct.h"
#include "resource_manger.h"

#include <allegro5/allegro5.h>

#include <iostream>

static inline void change_texture(const SpriteItem *t, ALLEGRO_EVENT_SOURCE *s)
{
    ALLEGRO_EVENT event;
    event.user.data1 = GameRenderCommand::GRC_CHANGE_TEXTURE;
    event.user.data2 = (intptr_t)t;
    al_emit_user_event(s, &event, nullptr);
}

class SCSMovement;
class SCSDisabled;

/*************************************************************************************************
 *                                                                                               *
 *                                BORN                                                           *
 *                                                                                               *
 *************************************************************************************************/

class SCSBornAnimed : public SCS
{
private:
    bool FILTER[STGCharCommand::SCC_NUM];
    int timer;

public:
    Anime Animation;
    int Duration;

    SCSMovement *Next;

    SCSBornAnimed()
    {
        for (int i = 0; i < STGCharCommand::SCC_NUM; i++)
            FILTER[i] = false;
        FILTER[STGCharCommand::SCC_UP] = true;
        FILTER[STGCharCommand::SCC_DOWN] = true;
        FILTER[STGCharCommand::SCC_LEFT] = true;
        FILTER[STGCharCommand::SCC_RIGHT] = true;
        FILTER[STGCharCommand::SCC_MOVE_XY] = true;
    };
    SCSBornAnimed(const SCSBornAnimed &) = delete;
    SCSBornAnimed(SCSBornAnimed &&) = delete;
    SCSBornAnimed &operator=(const SCSBornAnimed &) = delete;
    SCSBornAnimed &operator=(SCSBornAnimed &&) = delete;
    ~SCSBornAnimed() = default;

    void Init(const STGTexture &texs)
    {
        Animation = ResourceManager::GetAnime(texs.SpriteBorn);
        /* When use pending to change, make change happens in advance one frame. */
        Duration = Animation.LG_DURATION - 1;
        timer = -1;
    }

    void Copy(const SCSBornAnimed *o)
    {
        Animation = o->Animation;
        Animation.Reset();
        /* When use pending to change, make change happens in advance one frame. */
        Duration = Animation.LG_DURATION - 1;
        timer = -1;
    }

    bool CheckInput(ALLEGRO_EVENT *ie) noexcept final { return FILTER[ie->user.data1]; }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final { return false; }

    void Action(STGCharactor *sc) final
    {
        timer += 1;

#ifdef _DEBUG
        if (timer == 0)
            std::cout << "Charactor-" << sc->CodeName << " born.\n";
#endif

        /* Last animation frame (in logic) must return false. */
        if (timer < Duration)
        {
            sc->SPending = this;
            if (Animation.Forward())
                change_texture(Animation.Playing, sc->RendererMaster);
        }
        else
        {
#ifdef _DEBUG
            std::cout << "Charactor-" << sc->CodeName << " will transform to normal state.\n";
#endif

            timer = -1;
            Animation.Reset();
            sc->SPending = reinterpret_cast<SCS *>(Next);

            /* Notify */
            ALLEGRO_EVENT event;
            event.user.data1 = STGCharEvent::SCE_OPREATIONAL;
            al_emit_user_event(sc->KneeJump, &event, nullptr);
        }
    }
};

/*************************************************************************************************
 *                                                                                               *
 *                                          Disable                                              *
 *                                                                                               *
 *************************************************************************************************/

class SCSDisabled : public SCS
{
protected:
    int respwan_id;

public:
    SCSDisabled()
    {
        respwan_id = -1;
    };
    SCSDisabled(const SCSDisabled &) = delete;
    SCSDisabled(SCSDisabled &&) = delete;
    SCSDisabled &operator=(const SCSDisabled &) = delete;
    SCSDisabled &operator=(SCSDisabled &&) = delete;
    virtual ~SCSDisabled() = default;

    virtual void Action(STGCharactor *sc) override
    {
        if (sc->SNow != this)
        {
#ifdef _DEBUG
            std::cout << "Charactor-" << sc->CodeName << " disable (one frame).\n";
#endif

            sc->SPending = this;
            sc->SNow = this;
        }
        else
        {
            if (respwan_id < 0)
            {
#ifdef _DEBUG
                std::cout << "Charactor-" << sc->CodeName << " disable (one frame) end.\n";
#endif

                sc->Farewell();
            }
            else
            {
#ifdef _DEBUG
                std::cout << "Charactor-" << sc->CodeName
                          << " need to respwan (one frame): " << respwan_id << ".\n";
#endif

                sc->Con->HelpRespwan(respwan_id, sc->ID);
                respwan_id = -1;
            }
        }
    }

    bool CheckInput(ALLEGRO_EVENT *ie) final
    {
        if (ie->user.data1 == STGCharCommand::SCC_RESPAWN)
        {
            respwan_id = ie->user.data2;

#ifdef _DEBUG
            std::cout << "Will respwan form disable:" << respwan_id << "\n";
#endif
        }

        return false;
    }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final { return false; }
};

class SCSDisabledAnimed : public SCSDisabled
{
private:
    int timer;

public:
    Anime Animation;
    int Duration;

    SCSDisabledAnimed() = default;
    SCSDisabledAnimed(const SCSDisabledAnimed &) = delete;
    SCSDisabledAnimed(SCSDisabledAnimed &&) = delete;
    SCSDisabledAnimed &operator=(const SCSDisabledAnimed &) = delete;
    SCSDisabledAnimed &operator=(SCSDisabledAnimed &&) = delete;
    ~SCSDisabledAnimed() = default;

    void Init(const STGTexture &texs)
    {
        timer = -1;
        respwan_id = -1;
        Animation = ResourceManager::GetAnime(texs.SpriteDisable);
        Duration = Animation.LG_DURATION;
    }

    void Copy(const SCSDisabledAnimed *o)
    {
        timer = -1;
        respwan_id = -1;
        Animation = o->Animation;
        Duration = Animation.LG_DURATION;
    }

    virtual void Action(STGCharactor *sc) final
    {
        timer += 1;
        if (timer == 0)
        {
#ifdef _DEBUG
            std::cout << "Charactor-" << sc->CodeName << " disable.\n";
#endif

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
            if (respwan_id < 0)
            {
#ifdef _DEBUG
                std::cout << "Charactor-" << sc->CodeName << " disable end.\n";
#endif

                sc->Farewell();
            }
            else
            {
#ifdef _DEBUG
                std::cout << "Charactor-" << sc->CodeName
                          << " need to respwan: " << respwan_id << ".\n";
#endif

                sc->Con->HelpRespwan(respwan_id, sc->ID);
                sc->Physics->SetActive(true);
                respwan_id = -1;
            }

            timer = -1;
            Animation.Reset();
        }
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
    int PRIORTY[STGCharCommand::SCC_NUM];
    int next_i;
    int last_time_where;

    inline int move_check(const b2Vec2 v) const noexcept
    {
        int where = Movement::MM_IDLE;
        if (v.x > .1f)
            where += Movement::MM_TO_RIGHT;
        else if (v.x < -.1f)
            where += Movement::MM_TO_LEFT;
        if (v.y > .1f)
            where += Movement::MM_TO_DOWN;
        else if (v.y < -.1f)
            where += Movement::MM_TO_UP;
        return where;
    }

    inline void init() noexcept
    {
        next_i = STGCharCommand::SCC_UP;
        last_time_where = -1;

        for (int i = 0; i < STGCharCommand::SCC_NUM; i++)
            Next[i] = nullptr;
        NextDisable = nullptr;
    }

public:
    SCS *Next[STGCharCommand::SCC_NUM];
    SCSDisabled *NextDisable;

    SCSMovement()
    {
        for (int i = 0; i < STGCharCommand::SCC_NUM; i++)
            PRIORTY[i] = 0;
        PRIORTY[STGCharCommand::SCC_STG_CHANGE] = 9;
        PRIORTY[STGCharCommand::SCC_STG_SYNC] = 9;
        PRIORTY[STGCharCommand::SCC_STG_FORCE_SYNC_REQUEST] = 10;
        PRIORTY[STGCharCommand::SCC_STG_FORCE_SYNC_RESPONE] = 10;
        PRIORTY[STGCharCommand::SCC_DISABLE] = 999;
    }
    SCSMovement(const SCSMovement &) = delete;
    SCSMovement(SCSMovement &&) = delete;
    SCSMovement &operator=(const SCSMovement &) = delete;
    SCSMovement &operator=(SCSMovement &&) = delete;
    virtual ~SCSMovement() = default;

    bool CheckInput(ALLEGRO_EVENT *ie) noexcept final
    {
        if (PRIORTY[ie->user.data1] > PRIORTY[next_i])
            next_i = ie->user.data1;
        return PRIORTY[ie->user.data1] >= 0;
    }

    bool CheckChange(const STGChange *change, STGCharactor *sc) final
    {
        if (change->Code == STGStateChangeCode::SSCC_GO_DIE)
            NextDisable->Action(sc);
        return true;
    }

    virtual void Init(const STGTexture &texs) = 0;
    virtual void Copy(const SCSMovement *o) = 0;
};

class SCSMovementStatic : public SCSMovement
{
public:
    SpriteItem Texture[Movement::MM_NUM];

    SCSMovementStatic() = default;
    SCSMovementStatic(const SCSMovementStatic &) = delete;
    SCSMovementStatic(SCSMovementStatic &&) = delete;
    SCSMovementStatic &operator=(const SCSMovementStatic &) = delete;
    SCSMovementStatic &operator=(SCSMovementStatic &&) = delete;
    ~SCSMovementStatic() = default;

    void Init(const STGTexture &texs) final
    {
        init();
        for (int i = 0; i < Movement::MM_NUM; i++)
            Texture[i] = ResourceManager::GetTexture(texs.SpriteMovement[i]);
    }

    void Copy(const SCSMovement *o) final
    {
        init();
        std::memcpy(Texture, static_cast<const SCSMovementStatic *>(o)->Texture, sizeof(Texture));
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[next_i])
        {
            Next[next_i]->Action(sc);
            next_i = STGCharCommand::SCC_UP;
            last_time_where = -1;
        }
        else
        {
            sc->SPending = this;
            int where = move_check(sc->Velocity);
            if (last_time_where != where)
            {
                last_time_where = where;
                change_texture(Texture + where, sc->RendererMaster);
            }
        }
    }
};

class SCSMovementAnimed : public SCSMovement
{
public:
    Anime Animation[Movement::MM_NUM];

    SCSMovementAnimed() = default;
    SCSMovementAnimed(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed(SCSMovementAnimed &&) = delete;
    SCSMovementAnimed &operator=(const SCSMovementAnimed &) = delete;
    SCSMovementAnimed &operator=(SCSMovementAnimed &&) = delete;
    ~SCSMovementAnimed() = default;

    void Init(const STGTexture &texs) final
    {
        init();
        for (int i = 0; i < Movement::MM_NUM; i++)
            Animation[i] = ResourceManager::GetAnime(texs.SpriteMovement[i]);
    }

    void Copy(const SCSMovement *o) final
    {
        init();
        for (int i = 0; i < Movement::MM_NUM; i++)
        {
            Animation[i] = static_cast<const SCSMovementAnimed *>(o)->Animation[i];
            Animation[i].Reset();
        }
    }

    void Action(STGCharactor *sc) final
    {
        if (Next[next_i])
        {
            Next[next_i]->Action(sc);
            Animation[last_time_where].Reset();
            next_i = STGCharCommand::SCC_UP;
            last_time_where = -1;
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

#endif