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
#include "stg_shooter.h"

#ifdef STG_DEBUG_PHY_DRAW
#include "physical_draw.h"
#endif

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

enum class STGCompType
{
    CHARACTOR,
    RENDER,
    THINKER,
    SHOOTER,

    NUM,

    ALL
};

struct StageCharInfo
{
    STGCharactorSetting MyChar; /* same in airborne */
    STGShooter *MyShooters;     /* copy in airborne */
    SCPatternsCode MyPtn;       /* change in airborne */
    SCPatternData MyPD;         /* change in airborne */
    SCSBorn *MyEnter;           /* copy in airborne */
};

class STGLevel : public Scene, public STGFlowController
{
public:
    GameFlowController *GameCon;

    /* Store the Player's stg status. */
    StageCharRes SPlayer;
    StageCharInfo GPlayer;

    /* Constructor/Destructor */
    STGLevel();
    STGLevel(const STGLevel &) = delete;
    STGLevel(STGLevel &&) = delete;
    STGLevel &operator=(const STGLevel &) = delete;
    STGLevel &operator=(STGLevel &&) = delete;
    ~STGLevel() = default;

    /* Self constructor */
    void Load(int width, int height, float time_step, const STGLevelSetting &setting);
    void Unload();

    /* GameLoop */
    void Update();
    void Render(float forward_time);

    /* Provied Player's input terminal to input processor. */
    ALLEGRO_EVENT_QUEUE *InputConnectionTerminal() const noexcept;

    /* Stage flow control API */
    void FillMind(int char_id, SCPatternsCode ptn, SCPatternData pd) noexcept;
    void Debut(int char_id, float x, float y) final;
    void Airborne(int char_id, float x, float y, SCPatternsCode ptn, SCPatternData pd) final;
    void Pause() const final;
    void DisableAll(int id) final;
    void DisableThr(int id) final;
    void EnableSht(int id, STGShooter *ss) noexcept final;
    void DisableSht(int id, STGShooter *ss) noexcept final;

    const b2Body *TrackEnemy() final;

private:
    /* Also directly used in game. */
    STGStateMan all_state;

    /* Update things */
    STGThinker onstage_thinkers[MAX_ON_STAGE];
    int thinkers_n;
    STGCharactor onstage_charactors[MAX_ON_STAGE];
    int charactors_n;
    STGShooter many_shooters[MAX_ENTITIES * 2];
    STGShooter *shooters_p;
    int shooters_n;
    SpriteRenderer sprite_renderers[MAX_ON_STAGE];
    int sprite_renderers_n;

    /* Preload things */
    StageCharInfo standby[MAX_ENTITIES];

    /* Battle field */
    /* STG Field Buffer Area */
    static constexpr float STG_FIELD_BOUND_BUFFER = 5.f;
    float bound[4];
    float time_step;

    /* Physical things. */
#ifdef _DEBUG
    STGDebugContactListener d_contact_listener;
#endif
    b2World *world;
    b2BodyDef bd; /* Used for create char's physic in staging. */
    b2Body *player;

    /* Lua Things. */
    lua_State *L_stage;
    int stage_thread_ref;

    /* ID System */
    int records[MAX_ON_STAGE][static_cast<int>(STGCompType::NUM)];
    bool used_record[MAX_ON_STAGE];
    int record_hint;
    inline int get_id() noexcept;
    inline void return_id(int id) noexcept;
    inline void reset_id() noexcept;

    /* Memory management */
    int disabled[MAX_ON_STAGE * static_cast<int>(STGCompType::NUM)];
    STGCompType disabled_t[MAX_ON_STAGE * static_cast<int>(STGCompType::NUM)];
    int disabled_n;

    /* AUX Function */
    inline void process_pattern_data(SCPatternsCode ptn, SCPatternData &pd) const noexcept;
    STGShooter *copy_shooters(const STGShooter *first);

#ifdef STG_DEBUG_PHY_DRAW
    PhysicalDraw p_draw;
#endif
};

#endif
