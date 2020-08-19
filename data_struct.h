#ifndef GAME_DATA_STRUCT_H
#define GAME_DATA_STRUCT_H

#include "game_event.h"
#include "anime.h"

#include <lua.hpp>
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
    G_ENEMY_SIDE = -2,

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
    b2FixtureDef FD;
    ShapeType Shape;

    b2CircleShape C;
    b2PolygonShape P;
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

    bool FullMovementSprites;
    ALLEGRO_BITMAP *VeryFirstTex;
};

/*************************************************************************************************
 *                                                                                               *
 *                           Movement Type (Accelerate Pattern)                                  *
 *                                                                                               *
 *************************************************************************************************/

struct KinematicPhase
{
    float VV;
    float VR;
    int PhaseTime;   /* frame */
    float TransTime; /* sec. */
};

constexpr int MAX_KINEMATIC_PHASE_NUM = 8;

struct KinematicSeq
{
    bool Track;
    bool Loop;
    bool Dir;

    int SeqSize;
    KinematicPhase Seq[MAX_KINEMATIC_PHASE_NUM];
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

    KinematicSeq KS;
};

/*************************************************************************************************
 *                                                                                               *
 *                             STG SHooter Setting & Pattern                                     *
 *                   Unlike bullet: Pattern is not bind with Shooter!                            *
 *                                                                                               *
 *************************************************************************************************/

constexpr int MAX_LUNCHERS_NUM = 8;

enum class SSPatternsCode
{
    CONTROLLED,
    STAY,
    TOTAL_TURN,
    SPLIT_TURN,
    TRACK,

    NUM
};

union SSPatternData
{
    float TurnSpeed;
    float TurnSpeeds[MAX_LUNCHERS_NUM];
};

struct Attitude
{
    b2Vec2 Pos;
    float Angle;
};

struct Luncher
{
    Attitude DAttitude;
    int Interval;
    int AmmoSlot;
};

struct STGShooterSetting
{
    std::string Name;

    int AmmoSlotsNum;
    int Power;
    float Speed;

    int LuncherSize;
    Luncher Lunchers[MAX_LUNCHERS_NUM];

    SSPatternsCode Pattern;
    SSPatternData Data;
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
    GO_ROUND,

    NUM
};

union SCPatternData
{
    /// Default constructor does nothing (for performance).
    SCPatternData() {}

    lua_State *AI;

    b2Vec2 Vec;

    struct
    {
        b2Vec2 Vec[4];
        int Num;
        bool Loop;
    } Passby;

    struct
    {
        b2Vec2 p;
        float r_sq;
        float dir;
    } Round;
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
    std::string Text;
    ALLEGRO_FONT *Font;
    float X;
    float Y;
    int Align;
};

#endif