#ifndef STG_CONTACT_LISTENER
#define STG_CONTACT_LISTENER

#include "data_struct.h"

#include <box2d/box2d.h>

#include <iostream>

class CollisionHandler
{
public:
    CollisionHandler() = default;
    CollisionHandler(const CollisionHandler &) = delete;
    CollisionHandler(CollisionHandler &&) = delete;
    CollisionHandler &operator=(const CollisionHandler &) = delete;
    CollisionHandler &operator=(CollisionHandler &&) = delete;
    virtual ~CollisionHandler() = default;

    /* Called by contact listener with the other thing. */
    virtual void Hit(CollisionHandler *) = 0;
    /* Called by the other with its collision effection. */
    virtual void Hurt(const STGChange *) = 0;
};

class STGContactListener : public b2ContactListener
{
public:
    STGContactListener() = default;
    STGContactListener(const STGContactListener &) = delete;
    STGContactListener(STGContactListener &&) = delete;
    STGContactListener &operator=(const STGContactListener &) = delete;
    STGContactListener &operator=(STGContactListener &&) = delete;
    ~STGContactListener() = default;

    void BeginContact(b2Contact *c) final
    {
        const b2Fixture *fa = c->GetFixtureA();
        const b2Fixture *fb = c->GetFixtureB();
        CollisionHandler *a = reinterpret_cast<CollisionHandler *>(fa->GetBody()->GetUserData());
        CollisionHandler *b = reinterpret_cast<CollisionHandler *>(fb->GetBody()->GetUserData());

#ifdef _DEBUG
        std::cout << "Collision detected!\n";
#endif

        if (a != nullptr && b != nullptr)
        {
#ifdef _DEBUG
            std::cout << "Collision with effect!\n";
#endif

            a->Hit(b);
            b->Hit(a);
        }
    }
};

#endif