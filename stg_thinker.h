#ifndef STG_THINKER
#define STG_THINKER

#include "flow_controller.h"

#include <lua.hpp>
#include <allegro5/allegro5.h>

class STGThinker
{
public:
    int ID;
    STGFlowController *Con;

    /* Pass controller to AI coroutine */
    static void HandoverController(lua_State *L);

    /* Lua AI commands */
    void Move(int dir);

    STGThinker();
    STGThinker(const STGThinker &) = delete;
    STGThinker(STGThinker &&) = delete;
    STGThinker &operator=(const STGThinker &) = delete;
    STGThinker &operator=(STGThinker &&) = delete;
    ~STGThinker();
    void CPPSuckSwap(STGThinker &) noexcept;

    void Active(int id, lua_State *co) noexcept;
    void Think();

    ALLEGRO_EVENT_SOURCE *InputMaster;
    ALLEGRO_EVENT_QUEUE *Recv;

private:
    lua_State *ai;
};

#endif