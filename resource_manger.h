#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include "data_struct.h"
#include "anime.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <lua.hpp>

#include <unordered_map>
#include <string>

class ResourceManager
{
private:
    /* Some file path and ext. */
    static std::string path_to_art;
    static std::string path_to_stg_shooter;
    static std::string path_to_stg_charactor;
    static std::string path_to_stg_bullet;
    static std::string path_to_stg_level;
    // static std::string path_to_scene;

    /* Lua things. Share with Game obj. */
    static lua_State *L_main;

    /* Resource storage */
    static std::unordered_map<std::string, ALLEGRO_BITMAP *> textures;
    static std::unordered_map<std::string, Anime> animations;
    static std::vector<ALLEGRO_BITMAP *> sheets;
    static std::unordered_map<std::string, STGLevelSetting> stg_levels;
    static std::unordered_map<std::string, STGCharactorSetting> stg_charactors;
    static std::unordered_map<std::string, STGBulletSetting> stg_bullets;
    static std::unordered_map<std::string, STGShooterSetting> stg_shooters;
    static std::unordered_map<std::string, ALLEGRO_FONT *> fonts;

    /* aux function */
    static PhysicalFixture load_phyfix(const std::string &name, ShapeType *ret_sc);
    static STGTexture load_stg_texture(const std::string &name);
    static KinematicSeq load_kinematic_seq(const std::string &name);

public:
    /* The key of stg stage threads table. */
    static const char *STG_STAGE_FUNCTIONS_KEY;
    static const char *STG_SHOOT_FUNCTIONS_KEY;

    /* It is static */
    ResourceManager() = delete;
    ResourceManager(const ResourceManager &) = delete;
    ResourceManager(ResourceManager &&) = delete;
    ResourceManager &operator=(const ResourceManager &) = delete;
    ResourceManager &operator=(ResourceManager &&) = delete;
    ~ResourceManager() = delete;

    /* Init Lua */
    static void Init(lua_State *L);

    /* Texture */
    static void LoadTexture(const std::string &name);
    static void LoadSpriteSheet(const std::string &name);
    static ALLEGRO_BITMAP *GetTexture(const std::string &name);
    static const Anime &GetAnime(const std::string &name);

    /* STG Level */
    static void LoadSTGLevel(const std::string &name);
    static STGLevelSetting &GetSTGLevel(const std::string &name);
    /* STG Char */
    static void LoadSTGChar(const std::string &name);
    static STGCharactorSetting &GetSTGChar(const std::string &name);
    /* STG Bullet */
    static void LoadSTGBullet(const std::string &name);
    static STGBulletSetting &GetSTGBullet(const std::string &name);
    /* STG Shooter */
    static void LoadSTGShooter(const std::string &name);
    static STGShooterSetting &GetSTGShooter(const std::string &name);

    /* Save */
    static void LoadSave();
    /* Font */
    static void LoadFont();
    static ALLEGRO_FONT *GetFont(const std::string &name);

    /* other functions */
    static lua_State *GetCoroutine(const char *type, const std::string &name);
    // static void Clear();
};

#endif