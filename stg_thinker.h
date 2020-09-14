#ifndef STG_THINKER
#define STG_THINKER

#include "flow_controller.h"
#include "data_struct.h"

#include <lua.hpp>
#include <box2d/box2d.h>
#include <allegro5/allegro5.h>

#include <functional>

class STGThinker
{
    friend static int use_pattern(lua_State *L);
    friend static int move(lua_State *L);

public:
    int ID;
    STGFlowController *Con;

    /* Pass controller to AI coroutine */
    static void HandoverController(lua_State *L);
    /* Make STG Charactor Pattern functions */
    static void InitSCPattern();

    STGThinker();
    STGThinker(const STGThinker &) = delete;
    STGThinker(STGThinker &&) = delete;
    STGThinker &operator=(const STGThinker &) = delete;
    STGThinker &operator=(STGThinker &&) = delete;
    ~STGThinker();
    void CPPSuckSwap(STGThinker &) noexcept;

    void Active(int id, SCPatternsCode ptn, SCPatternData pd, const b2Body *body) noexcept;
    void Think();

    ALLEGRO_EVENT_SOURCE *InputMaster;
    ALLEGRO_EVENT_QUEUE *Recv;

    struct
    {
        int WaitTime;
        unsigned int EventBits;
        unsigned int NotifyMask;
    } Communication;

private:
    const b2Body *physics;

    int where;
    b2Vec2 vec4u;
    SCPatternData data;
    std::function<bool(STGThinker *)> pattern;

    /* Lua AI can set sub-pattern, also use data. */
    lua_State *ai;
    SCPatternsCode sub_ptn;

    static std::function<bool(STGThinker *)> patterns[SCPatternsCode::SCPC_NUM];
    bool controlled();
    bool move_to();
    bool move_last();
    bool move_passby();
    bool go_round();
};

#endif