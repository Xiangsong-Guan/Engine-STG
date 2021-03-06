#ifndef GAME_DATA_STRUCT_H
#define GAME_DATA_STRUCT_H

#include "game_event.h"

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

enum ShapeType
{
    ST_CIRCLE,
    ST_BOX
};

const char *const SHAPE_TYPE[] = {"CIRCLE", "BOX", NULL};

enum SpriteType
{
    SPT_STATIC,
    SPT_ANIMED,

    SPT_NUM,

    SPT_NONE
};

enum CollisionType
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
 *                                   Sprite Render Setting                                       *
 *                                                                                               *
 *************************************************************************************************/

struct SpriteItem
{
    ALLEGRO_BITMAP *Sprite;
    float CX;
    float CY;
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
    std::string SpriteMovement[Movement::MM_NUM];
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

    SpriteItem VeryFirstTex;
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
    int PhaseTime; /* frame */
    float VAcceleration;
    float RAcceleration;
    int TransEnd; /* PhaseTime + TransTime * UPDATE_PRE_SEC */
};

constexpr int MAX_KINEMATIC_PHASE_NUM = 16;

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
 *                             STG SHooter Setting & Pattern                                     *
 *                   Unlike bullet: Pattern is not bind with Shooter!                            *
 *                                                                                               *
 *************************************************************************************************/

constexpr int MAX_LUNCHERS_NUM = 8;

enum SSPatternsCode
{
    SSPC_CONTROLLED,
    SSPC_STAY,
    SSPC_TOTAL_TURN,
    SSPC_SPLIT_TURN,
    SSPC_TRACK,

    SSPC_NUM
};

const char *const SS_PATTERNS_CODE[] =
    {"CONTROLLED",
     "STAY",
     "TOTAL_TURN",
     "SPLIT_TURN",
     "TRACK",
     NULL};

union SSPatternData
{
    float turn_speed;
    float turn_speeds[MAX_LUNCHERS_NUM];
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
    std::string CodeName;

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
    std::string CodeName;

    float DefaultSpeed;

    PhysicalFixture Phy;
    STGTexture Texs;
};

enum SCPatternsCode
{
    SCPC_CONTROLLED,
    SCPC_STAY,
    SCPC_MOVE_TO,
    SCPC_MOVE_LAST,
    SCPC_MOVE_PASSBY,
    SCPC_GO_ROUND,

    SCPC_NUM
};

const char *const SC_PATTERNS_CODE[] =
    {"CONTROLLED",
     "STAY",
     "MOVE_TO",
     "MOVE_LAST",
     "MOVE_PASSBY",
     "GO_ROUND",
     NULL};

union SCPatternData
{
    /// Default constructor does nothing (for performance).
    SCPatternData() {}

    lua_State *ai;

    b2Vec2 vec;

    struct
    {
        b2Vec2 Vec[4];
        int Num;
        bool Loop;
    } passby;

    struct
    {
        b2Vec2 P;
        float R_SQ;
        float Dir;
    } round;
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
    std::string CodeName;

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

/*************************************************************************************************
 *                                                                                               *
 *                               STG Status Change / State Type                                  *
 *                                                                                               *
 *************************************************************************************************/

enum STGStateChangeCode
{
    SSCC_JUST_HURT,
    SSCC_GO_DIE,

    SSCC_NUM
};

const char *const STG_STATE_CHANGE_CODE[] = {"JUST_HURT", "GO_DIE", NULL};

/* SHOULD ALWAYS BE CONTAINED AT FIRST */
#define STGCHANGE_HEADER     \
    STGStateChangeCode Code; \
    int Damage;

union STGChange
{
    /* NOT OPTIONAL */
    STGStateChangeCode Code;

    /* FOR EXAMPLE */
    struct
    {
        STGCHANGE_HEADER;
    } any;

    /* Just get some bullets to eat, nothing. */
    struct
    {
        STGCHANGE_HEADER;
    } just_hurt;
};

/*************************************************************************************************
 *                                                                                               *
 *                               STG Bullet Setting & Pattern                                    *
 *                                                                                               *
 *************************************************************************************************/

struct STGBulletSetting
{
    std::string Name;
    std::string CodeName;

    PhysicalFixture Phy;

    STGTexture Texs;

    STGChange Change;

    KinematicSeq KS;
};

#endif