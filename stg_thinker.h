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
public:
    int ID;
    STGFlowController *Con;

    /* Pass controller to AI coroutine */
    static void HandoverController(lua_State *L);
    /* Make STG Charactor Pattern functions */
    static void InitSCPattern();

    /* Lua AI commands */
    void Move(int dir);

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

private:
    const b2Body *physics;

    int where;
    b2Vec2 vec4u;
    SCPatternData data;
    std::function<bool(STGThinker *)> pattern;

    /* Lua AI can set sub-pattern, also use data. */
    lua_State *AI;
    SCPatternsCode sub_ptn;

    static std::function<bool(STGThinker *)> patterns[static_cast<int>(SCPatternsCode::NUM)];
    bool controlled();
    bool move_to();
    bool move_last();
    bool move_passby();
};

#endif