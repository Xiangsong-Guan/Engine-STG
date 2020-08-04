#ifndef STG_LEVEL_H
#define STG_LEVEL_H

#include "scene.h"
#include "flow_controller.h"
#include "data_struct.h"
#include "sprite_renderer.h"
#include "stg_charactor.h"
#include "stg_thinker.h"
#include "stg_pattern.h"
#include "stg_state_man.h"

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <vector>

enum class STGCompType
{
    CHARACTOR,
    RENDER,
    THINKER,
    PATTERN,

    NUM,

    ALL
};

class STGLevel : public Scene, public STGFlowController
{
public:
    /* Store the Player's stg status. */
    STGCharactorSetting Gamer;

    /* Constructor/Destructor */
    STGLevel() = default;
    STGLevel(const STGLevel &) = delete;
    STGLevel(STGLevel &&) = delete;
    STGLevel &operator=(const STGLevel &) = delete;
    STGLevel &operator=(STGLevel &&) = delete;
    ~STGLevel() = default;

    /* Self constructor */
    void Load(int width, int height, float time_step,
              const STGLevelSetting &setting, GameFlowController *c);
    void Unload();

    /* GameLoop */
    void Update();
    void Render(float forward_time);

    /* Provied Player's input terminal to input processor. */
    ALLEGRO_EVENT_QUEUE *InputConnectionTerminal() const noexcept;

    /* Stage flow control API */
    void FillMind(int char_id, SCPatternsCode ptn, SCPatternData pd) noexcept;
    void Brain(int char_id, lua_State *co) noexcept;
    virtual void Debut(int char_id, float x, float y) final;
    virtual void Airborne(int char_id, float x, float y, lua_State *co) final;
    virtual void Airborne(int char_id, float x, float y,
                          SCPatternsCode ptn, SCPatternData pd) final;
    virtual void Pause() const final;
    virtual void DisableAll(int id) final;
    virtual void DisablePtn(int id) final;
    virtual void DisableThr(int id) final;

private:
    /* Physics sim setting */
    static constexpr int VELOCITY_ITERATIONS = 6;
    static constexpr int POSITION_ITERATIONS = 2;

    /* Limit */
    static constexpr int MAX_ENTITIES = 2048u;
    static constexpr int MAX_ON_STAGE = 256u;

    /* Preload things */
    STGCharactorSetting my_charactor[MAX_ENTITIES];
    lua_State *my_thinker[MAX_ENTITIES];
    SCPatternsCode my_pattern[MAX_ENTITIES];
    SCPatternData my_pattern_data[MAX_ENTITIES];
    SCS *my_enter[MAX_ENTITIES];

    /* Also directly used in game. */
    STGStateMan all_state;

    /* Update things */
    STGCharactor onstage_charactors[MAX_ON_STAGE];
    int charactors_n;
    STGThinker onstage_thinkers[MAX_ON_STAGE];
    int thinkers_n;
    SCPattern onstage_patterns[MAX_ON_STAGE];
    int patterns_n;
    SpriteRenderer sprite_renderers[MAX_ON_STAGE];
    int sprite_renderers_n;

    /* Battle field */
    /* STG Field Buffer Area */
    static constexpr float STG_FIELD_BOUND_BUFFER = 5.f;
    int width, height;
    float physical_width, physical_height;
    float bound[4];
    float time_step;

    /* Physical things. */
    b2World *world;
    b2BodyDef bd; /* Used for create char's physic in staging. */
    b2Body *player;

    /* Lua Things. */
    lua_State *L_stage;
    int stage_thread_ref;

    GameFlowController *con;

    /* ID System */
    int records[MAX_ON_STAGE][static_cast<int>(STGCompType::NUM)];
    bool used_record[MAX_ON_STAGE];
    int record_hint;
    int get_id() noexcept;
    void return_id(int id) noexcept;
    void reset_id() noexcept;
};

#endif
