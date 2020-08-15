#ifndef STG_CONTACT_LISTENER
#define STG_CONTACT_LISTENER

#include <box2d/box2d.h>

#include <iostream>

class STGDebugContactListener : public b2ContactListener
{
public:
    STGDebugContactListener() = default;
    STGDebugContactListener(const STGDebugContactListener &) = delete;
    STGDebugContactListener(STGDebugContactListener &&) = delete;
    STGDebugContactListener &operator=(const STGDebugContactListener &) = delete;
    STGDebugContactListener &operator=(STGDebugContactListener &&) = delete;
    ~STGDebugContactListener() = default;

    void BeginContact(b2Contact *c) final
    {
        std::cout << "BOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOM";
        std::cout << std::endl;
    }
};

#endif