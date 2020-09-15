#ifndef STG_LEVEL_H
#define STG_LEVEL_H

#include "scene.h"
#include "flow_controller.h"
#include "data_struct.h"
#include "sprite_renderer.h"
#include "stg_charactor.h"
#include "stg_thinker.h"
#include "stg_state_man.h"
#include "contact_listener.h"
#include "shooter.h"
#include "bullet.h"

#ifdef STG_DEBUG_PHY_DRAW
#include "physical_draw.h"
#endif
#ifdef STG_PERFORMENCE_SHOW
#include "text_renderer.h"
#endif

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <string>

enum STGCompType
{
    SCT_CHARACTOR,
    SCT_RENDER,
    SCT_THINKER,

    /* Count in for management. */
    SCT_NUM,

    SCT_SHOOTER
};

struct StageCharInfo
{
    STGCharactorSetting MyChar; /* same in airborne */
    Shooter *MyShooters;        /* copy in airborne */
    SCPatternsCode MyPtn;       /* change in airborne */
    SCPatternData MyPD;         /* change in airborne */
    SCS *MyEnter;               /* copy in airborne */
};

class STGLevel : public Scene, public STGFlowController
{
public:
    std::string Name;
    std::string CodeName;

    GameFlowController *GameCon;
    ALLEGRO_EVENT_QUEUE *PlayerInputTerminal;

    /* Store the Player's stg status. */
    StageCharRes SPlayer;
    StageCharInfo GPlayer;

    /* Constructor/Destructor */
    STGLevel();
    STGLevel(const STGLevel &) = delete;
    STGLevel(STGLevel &&) = delete;
    STGLevel &operator=(const STGLevel &) = delete;
    STGLevel &operator=(STGLevel &&) = delete;
    ~STGLevel();

    /* Self constructor */
    void Load(float time_step, const STGLevelSetting &setting);
    void Unload();

    /* GameLoop */
    void Update();
    void Render(float forward_time);

    /* Stage flow control API */
    void FillMind(int char_id, SCPatternsCode ptn, SCPatternData pd) noexcept;
    void Debut(int char_id, float x, float y) final;
    void Airborne(int char_id, float x, float y, SCPatternsCode ptn, SCPatternData pd) final;
    void EnableBullet(Bullet *b) final;
    void DisableBullet(Bullet *b) final;
    void Pause() const final;
    void DisableAll(int id) final;
    void DisableThr(int id) final;
    void EnableSht(Shooter *ss) noexcept final;
    void DisableSht(Shooter *ss) noexcept final;

    const b2Body *TrackEnemy() const noexcept final;
    const b2Body *TrackPlayer() const noexcept final;

    void HelpRespwan(int preload_id, int real_id) final;
    void PlayerWin();
    void PlayerDying();

private:
    /* Also directly used in game. */
    STGStateMan all_state;

    /* Update things */
    STGThinker onstage_thinkers[MAX_ON_STAGE];
    int thinkers_n;
    STGCharactor onstage_charactors[MAX_ON_STAGE];
    int charactors_n;
    Shooter many_shooters[MAX_ENTITIES * 2];
    Shooter *shooters_p;
    int shooters_n;
    Bullet bullets[MAX_ENTITIES / 2];
    Bullet *bullets_p;
    int bullets_n;
    SpriteRenderer sprite_renderers[MAX_ON_STAGE];
    int sprite_renderers_n;

    /* Preload things */
    StageCharInfo standby[MAX_ENTITIES];

    /* Battle field */
    /* STG Field Buffer Area */
    float bound[4];
    float time_step;

    /* Physical things. */
    STGContactListener contact_listener;
    b2World *world;
    b2BodyDef bd; /* Used for create char's physic in staging. */
    b2Body *player;

    /* Lua Things. */
    lua_Integer timer;
    lua_State *L_stage;

    /* Count down. */
    ALLEGRO_TIMER *count_down;

    /* ID System */
    int records[MAX_ON_STAGE][STGCompType::SCT_NUM];
    bool used_record[MAX_ON_STAGE];
    int record_hint;
    inline int get_id() noexcept;
    inline void return_id(int id) noexcept;
    inline void reset_id() noexcept;

    /* Memory management */
    int disabled[MAX_ON_STAGE * STGCompType::SCT_NUM];
    STGCompType disabled_t[MAX_ON_STAGE * STGCompType::SCT_NUM];
    int disabled_n;

    /* AUX Function */
    Shooter *copy_shooters(const Shooter *first);

#ifdef STG_DEBUG_PHY_DRAW
    PhysicalDraw p_draw;
#endif
#ifdef STG_PERFORMENCE_SHOW
    TextRenderer tr_bullets_n, tr_bn;
#endif
};

#endif
