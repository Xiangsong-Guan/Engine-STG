#ifndef STG_CONTACT_LISTENER
#define STG_CONTACT_LISTENER

#include <box2d/box2d.h>

#include <vector>

class STGDebugContactListener : public b2ContactListener
{
public:
    STGDebugContactListener() = default;
    STGDebugContactListener(const STGDebugContactListener &) = delete;
    STGDebugContactListener(STGDebugContactListener &&) = delete;
    STGDebugContactListener &operator=(const STGDebugContactListener &) = delete;
    STGDebugContactListener &operator=(STGDebugContactListener &&) = delete;
    ~STGDebugContactListener() = default;

    std::vector<std::pair<b2Fixture *, b2Fixture *>> ContactPairs;

    void BeginContact(b2Contact *c) final
    {
        ContactPairs.emplace_back(c->GetFixtureA(), c->GetFixtureB());
    }
};

#endif