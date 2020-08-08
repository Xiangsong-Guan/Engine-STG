#ifndef GAME_DATA_STRUCT_H
#define GAME_DATA_STRUCT_H

#include "game_event.h"

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>

#include <vector>
#include <string>

/*************************************************************************************************
 *                                                                                               *
 *                                     Setting Types                                             *
 *                                                                                               *
 *************************************************************************************************/

enum class ShapeType
{
    CIRCLE,
    BOX
};

enum class SpriteType
{
    STATIC,
    ANIMED,
    NONE
};

enum class CollisionType
{
    /* Groups */
    G_PLAYER_SIDE = -1,
    G_OTHERS_SIDE = -2,

    /* Categories */
    C_PLAYER = 0b0000000000000001,
    C_ENEMY = 0b0000000000000010,
    C_PLAYER_BULLET = 0b1000000000000000,
    C_ENEMY_BULLET = 0b0100000000000000,

    /* Mask often used */
    M_ALL_ENEMY = C_ENEMY | C_ENEMY_BULLET,
    M_ALL_PLAYER = C_PLAYER | C_PLAYER_BULLET
};

/*************************************************************************************************
 *                                                                                               *
 *                                     Phyiscal Fixture Setting                                  *
 *                                                                                               *
 *************************************************************************************************/

struct PhysicalFixture
{
    /* Physical */
    bool Physical;
    b2CircleShape C;
    b2PolygonShape P;
    b2FixtureDef FD;
};

/*************************************************************************************************
 *                                                                                               *
 *                                  STG Used Texture Group                                       *
 *                                                                                               *
 *************************************************************************************************/

struct STGTexture
{
    std::string SpriteBorn;
    SpriteType SpriteBornType;
    std::string SpriteMovement[static_cast<int>(Movement::NUM)];
    SpriteType SpriteMovementType;
    std::string SpriteShooting;
    SpriteType SpriteShootingType;
    std::string SpriteShift;
    SpriteType SpriteShiftType;
    std::string SpriteSync;
    SpriteType SpriteSyncType;
    std::string SpriteFSync;
    SpriteType SpriteFSyncType;
    std::string SpriteHit;
    SpriteType SpriteHitType;
    std::string SpriteDisable;
    SpriteType SpriteDisableType;

    ALLEGRO_BITMAP *VeryFirstTex;
};

/*************************************************************************************************
 *                                                                                               *
 *                           Movement Type (Accelerate Pattern)                                  *
 *                                                                                               *
 *************************************************************************************************/

enum class AccelerateType
{
    UNIFORM,
};

struct KinematicPhase
{
    float V;
    int PhaseTime;   /* frame */
    float TransTime; /* sec. */
    AccelerateType AT;
};

struct KinematicSeq
{
    bool Loop;

    std::vector<KinematicPhase> SpeedSeq;
    std::vector<KinematicPhase> AngleSeq;
};

/*************************************************************************************************
 *                                                                                               *
 *                               STG Bullet Setting & Pattern                                    *
 *                                                                                               *
 *************************************************************************************************/

struct STGBulletSetting
{
    std::string Name;

    PhysicalFixture Phy;

    STGTexture Texs;

    int Damage;
    float Speed;

    bool Track;
    KinematicSeq KinematicPhases;
};

/*************************************************************************************************
 *                                                                                               *
 *                               STG Bullet Setting & Pattern                                    *
 *                                                                                               *
 *************************************************************************************************/

struct Luncher
{
    float DAngle;
    float DPosX;
    float DPosY;
    int Interval;
    int AmmoSlot;
};

struct STGShooterSetting
{
    std::string Name;

    STGTexture Texs;

    int AmmoSlotsNum;
    int Power;
    int Speed;

    std::vector<Luncher> Lunchers;

    bool Track;
    KinematicSeq KinematicPhases;
};

/*************************************************************************************************
 *                                                                                               *
 *                           STG Charactor Setting & Pattern                                     *
 *                   Unlike bullet: Pattern is not bind with Charactor!                          *
 *                                                                                               *
 *************************************************************************************************/

struct STGCharactorSetting
{
    std::string Name;

    int DefaultSpeed;
    PhysicalFixture Phy;
    STGTexture Texs;
};

enum class SCPatternsCode
{
    CONTROLLED,
    STAY,
    MOVE_TO,
    MOVE_LAST,
    MOVE_PASSBY,

    NUM
};

union SCPatternData
{
    struct
    {
        float X;
        float Y;
    } Vec;

    struct
    {
        struct
        {
            float X;
            float Y;
        } Vec[4];
        int Where;
        int Num;
        bool Loop;
    } Passby;
};

/*************************************************************************************************
 *                                                                                               *
 *                                       STG Level Setting                                       *
 *                                                                                               *
 *************************************************************************************************/

struct StageCharRes
{
    std::string Char;
    std::vector<std::string> Shooters;
    std::vector<std::string> Bulletss;
};

struct STGLevelSetting
{
    std::string Name;

    /* About all charactors will on stage. */
    std::vector<StageCharRes> Charactors;
};

/*************************************************************************************************
 *                                                                                               *
 *                                     Text Render Setting                                       *
 *                                                                                               *
 *************************************************************************************************/

struct TextItem
{
    std::u8string Text;
    ALLEGRO_FONT *Font;
    float X;
    float Y;
    int Align;
};

#endif