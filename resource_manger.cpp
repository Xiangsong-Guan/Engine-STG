#include "resource_manger.h"

#include "cppsuckdef.h"

#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cmath>

inline int luaNOERROR_checkoption(lua_State *L, int arg, const char *def, const char *const lst[])
{
    /* From lauxlib.c */
    const char *name = (def) ? luaL_optstring(L, arg, def) : luaL_checkstring(L, arg);
    int i;
    for (i = 0; lst[i]; i++)
        if (strcmp(lst[i], name) == 0)
            return i;

    return -1;
}

/* Instantiate static variables */
std::unordered_map<std::string, ALLEGRO_BITMAP *> ResourceManager::textures;
std::unordered_map<std::string, Anime> ResourceManager::animations;
std::vector<ALLEGRO_BITMAP *> ResourceManager::sheets;
std::unordered_map<std::string, STGLevelSetting> ResourceManager::stg_levels;
std::unordered_map<std::string, STGCharactorSetting> ResourceManager::stg_charactors;
std::unordered_map<std::string, STGBulletSetting> ResourceManager::stg_bullets;
std::unordered_map<std::string, STGShooterSetting> ResourceManager::stg_shooters;
std::unordered_map<std::string, ALLEGRO_FONT *> ResourceManager::fonts;

/* some path */
std::string ResourceManager::path_to_art = "art/";
std::string ResourceManager::path_to_stg_shooter = "stg_shooter/";
std::string ResourceManager::path_to_stg_charactor = "stg_charactor/";
std::string ResourceManager::path_to_stg_bullet = "stg_bullet/";
std::string ResourceManager::path_to_stg_level = "stg_level/";
// std::string ResourceManager::path_to_scene = ResourceManager::path_to_res + "scene/";

/* Initialize Lua State. Can use this function read initialize setting? */
lua_State *ResourceManager::L_main = nullptr;
const char *ResourceManager::STG_STAGE_FUNCTIONS_KEY = "STG_STAGE_FUNCTIONS";
const char *ResourceManager::STG_SHOOT_FUNCTIONS_KEY = "STG_SHOOT_FUNCTIONS";
static int simple_lmod_loader(lua_State *L)
{
    luaL_dofile(L, lua_tostring(L, 1));
    lua_remove(L, 1);
    return 1;
}
void ResourceManager::Init(lua_State *L)
{
    L_main = L;
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, STG_STAGE_FUNCTIONS_KEY);
    lua_newtable(L);
    lua_setfield(L, LUA_REGISTRYINDEX, STG_SHOOT_FUNCTIONS_KEY);

    luaL_requiref(L, "hell/enum.lua", simple_lmod_loader, 0);
    lua_pop(L, 1);
}

void ResourceManager::LoadTexture(const std::string &name)
{
    /* Repeat load check */
    if (textures.contains(name))
        return;

    ALLEGRO_BITMAP *image = al_load_bitmap((path_to_art + name + ".png").c_str());
    if (image == nullptr)
        std::cerr << "Couldn't load image: " << name << std::endl;
    else
        textures.emplace(name, image);
}

ALLEGRO_BITMAP *ResourceManager::GetTexture(const std::string &name)
{
    return textures.at(name);
}

void ResourceManager::LoadSpriteSheet(const std::string &name)
{
    ALLEGRO_BITMAP *anime_sheet = nullptr;
    std::string file = path_to_art + name + ".json";

    try
    {
        /* load json */
        nlohmann::json anime_setting;
        (std::ifstream(file) >> anime_setting).close();

        /* find true anime sheet */
        std::string sheet_file = std::filesystem::path(file).parent_path().string() + "/" +
                                 anime_setting.at("meta").at("image").get<std::string>();

        /* load anime sheet */
        anime_sheet = al_load_bitmap(sheet_file.c_str());
        if (anime_sheet == nullptr)
        {
            std::cerr << "Failed to load image: " << sheet_file << std::endl;
            return;
        }
        sheets.push_back(anime_sheet);

        /* load each tag */
        const auto &frames = anime_setting.at("frames");
        for (const auto &tag : anime_setting["meta"].at("frameTags"))
        {
            std::string tag_name = name + "_" + tag.at("name").get<std::string>();

            /* Just one frame, load as statical texture when not loaded. */
            if (tag.at("from").get<int>() == tag.at("to").get<int>())
            {
                /* checking repeat load */
                if (textures.contains(tag_name))
                    continue;

                const auto &frame = frames[tag["from"].get<int>()].at("frame");
                ALLEGRO_BITMAP *sub_image =
                    al_create_sub_bitmap(anime_sheet,
                                         frame.at("x"), frame.at("y"),
                                         frame.at("w"), frame.at("h"));
                if (sub_image == nullptr)
                {
                    std::cerr << "Failed to create sub-image for " << file << std::endl;
                    return;
                }

                textures.emplace(tag_name, sub_image);
            }
            else
            {
                /* checking repeat load */
                if (animations.contains(tag_name))
                    continue;

                /* load this tag anime */
                std::vector<ALLEGRO_BITMAP *> ani;
                for (int i = tag["from"]; i <= tag["to"]; i++)
                {
                    int dur = frames[i].at("duration") / MS_PRE_ANIME_FRAME;
                    const auto &frame = frames[i].at("frame");
                    ALLEGRO_BITMAP *sub_image =
                        al_create_sub_bitmap(anime_sheet,
                                             frame.at("x"), frame.at("y"),
                                             frame.at("w"), frame.at("h"));
                    if (sub_image == nullptr)
                    {
                        std::cerr << "Failed to create sub-image for " << file << std::endl;
                        return;
                    }
                    ani.insert(ani.end(), dur, sub_image);
                }
                if (tag.at("direction") == "pingpong")
                {
                    size_t mid = ani.size();
                    ani.resize(ani.size() * 2);
                    std::reverse_copy(ani.begin(), ani.begin() + mid, ani.begin() + mid);
                }
                else if (tag["direction"] == "reverse")
                    std::reverse(ani.begin(), ani.end());

                ani.shrink_to_fit();
                animations.emplace(tag_name, Anime(std::move(ani)));
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Cannot load texture sheet " << name << ": " << e.what() << '\n';
    }
}

const Anime &ResourceManager::GetAnime(const std::string &name)
{
    return animations.at(name);
}

void ResourceManager::LoadSTGLevel(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif
    int balance_top = lua_gettop(L_main);

    /* Reloaded check. */
    if (stg_levels.contains(name))
        return;

    if (luaL_dofile(L_main, (path_to_stg_level + name + ".lua").c_str()))
    {
        std::cerr << "Failed to load STG Level " << name << ": "
                  << lua_tostring(L_main, -1) << std::endl;
        lua_pop(L_main, 1);
        return;
    }
    /* STG level lua script will return two value: first is the stage script as function,
     * second is resource need to be preload. */
    if (!lua_istable(L_main, -1))
    {
        std::cerr << "Failed to load STG Level: Level setting file not return table.\n";
        lua_settop(L_main, balance_top);
        return;
    }
    if (!lua_isfunction(L_main, -2))
    {
        std::cerr << "Failed to load STG Level: Level setting file not return stage function.\n";
        lua_settop(L_main, balance_top);
        return;
    }

    STGLevelSetting ls;
    ls.CodeName = name;

    /* Load all Chatactors. */
    if (lua_getfield(L_main, -1, "charactors") != LUA_TTABLE)
    {
        std::cerr << "Failed to load STG Level: Charactors setting not a list.\n";
        lua_settop(L_main, balance_top);
        return;
    }
    for (int i = 0; i < luaL_len(L_main, -1); i++)
    {
        StageCharRes scr;
        if (lua_geti(L_main, -1, i + 1) != LUA_TTABLE)
        {
            std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                      << "is invalid." << std::endl;
            lua_settop(L_main, balance_top);
            return;
        }

        /* Charactor */
        if (lua_getfield(L_main, -1, "name") != LUA_TSTRING)
        {
            std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                      << "'s resource ref is invalid." << std::endl;
            lua_settop(L_main, balance_top);
            return;
        }
        scr.Char = lua_tostring(L_main, -1);
        LoadSTGChar(scr.Char);
        lua_pop(L_main, 1);

        std::string name;

        /* Shooter */
        if (lua_getfield(L_main, -1, "shooters") != LUA_TTABLE)
        {
            std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                      << "'s shooters is not a list." << std::endl;
            lua_settop(L_main, balance_top);
            return;
        }
        for (int j = 0; j < luaL_len(L_main, -1); j++)
        {
            if (lua_geti(L_main, -1, j + 1) != LUA_TTABLE)
            {
                std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                          << "'s shooter-" << j + 1 << " is invalid." << std::endl;
                lua_settop(L_main, balance_top);
                return;
            }

            if (lua_getfield(L_main, -1, "name") != LUA_TSTRING)
            {
                std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                          << "'s shooter-" << j + 1 << "'s name is invalid." << std::endl;
                lua_settop(L_main, balance_top);
                return;
            }
            name = lua_tostring(L_main, -1);
            LoadSTGShooter(name);
            scr.Shooters.push_back(name);
            lua_pop(L_main, 1);

            /* Bullet */
            if (lua_getfield(L_main, -1, "bullets") != LUA_TTABLE)
            {
                std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                          << "'s shooter-" << j + 1 << "'s bullets is invalid." << std::endl;
                lua_settop(L_main, balance_top);
                return;
            }
            for (int k = 0; k < luaL_len(L_main, -1); k++)
            {
                if (lua_geti(L_main, -1, k + 1) != LUA_TSTRING)
                {
                    std::cerr << "Failed to load STG Level: Charactor-" << i + 1
                              << "'s shooter-" << j + 1 << "'s bullet-" << k + 1
                              << " is invalid." << std::endl;
                    lua_settop(L_main, balance_top);
                    return;
                }
                name = lua_tostring(L_main, -1);
                LoadSTGBullet(name);
                scr.Bulletss.push_back(name);
                lua_pop(L_main, 1);
            }
            lua_pop(L_main, 1);
            scr.Bulletss.shrink_to_fit();

            lua_pop(L_main, 1);
        }
        scr.Shooters.shrink_to_fit();
        lua_pop(L_main, 1);

        lua_pop(L_main, 1);
        ls.Charactors.push_back(std::move(scr));
    }

    /* Clean the resource stuff from stack. */
    lua_pop(L_main, 2); /* resource table and charactors list */
    /* Store the setting, need for later use. */
    ls.Charactors.shrink_to_fit();
    stg_levels.emplace(name, std::move(ls));

    /* Store the stage function with stg level setting's memory address in resource manger. */
    lua_getfield(L_main, LUA_REGISTRYINDEX, STG_STAGE_FUNCTIONS_KEY);
    lua_pushvalue(L_main, -2);
    lua_setfield(L_main, -2, name.c_str());
    lua_pop(L_main, 2);

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif
}

STGLevelSetting &ResourceManager::GetSTGLevel(const std::string &name)
{
    return stg_levels.at(name);
}

lua_State *ResourceManager::GetCoroutine(const char *type, const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif

    lua_State *ret;

    lua_getfield(L_main, LUA_REGISTRYINDEX, type);
    if (lua_getfield(L_main, -1, name.c_str()) != LUA_TFUNCTION)
        throw "no such lua function";
    lua_remove(L_main, -2);
    ret = lua_newthread(L_main);
    lua_pop(L_main, 1);
    lua_xmove(L_main, ret, 1);

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif

    return ret;
}

void ResourceManager::LoadSTGChar(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif

    /* Repeat Check */
    if (stg_charactors.contains(name))
        return;

    if (luaL_dofile(L_main, (path_to_stg_charactor + name + ".lua").c_str()))
    {
        std::cerr << "Failed to load STG Char " << name << ": "
                  << lua_tostring(L_main, -1) << std::endl;
        lua_pop(L_main, 1);
        return;
    }

    /* Lua will return a table for char. */
    if (!lua_istable(L_main, -1))
    {
        std::cerr << "Failed to load STG Char " << name << ": "
                  << "Lua return not valid!\n";
        lua_pop(L_main, 1);
        return;
    }

    STGCharactorSetting cs;
    cs.CodeName = name;
    cs.Phy = load_phyfix(name);
    cs.Texs = load_stg_texture(name);

    if (lua_getfield(L_main, -1, "default_speed") != LUA_TNUMBER)
    {
        std::cerr << "Failed to load STG Char " << name << ": "
                  << "invalid default speed!\n";
        lua_pop(L_main, 2);
        return;
    }
    cs.DefaultSpeed = lua_tonumber(L_main, -1);
    lua_pop(L_main, 1);

    /* All charactors is sensor. */
    cs.Phy.FD.isSensor = true;

    lua_pop(L_main, 1);
    stg_charactors.emplace(name, std::move(cs));

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif
}

STGCharactorSetting &ResourceManager::GetSTGChar(const std::string &name)
{
    return stg_charactors.at(name);
}

#define INVALID_BULLET(name, why, un)                               \
    {                                                               \
        lua_settop(L_main, (un));                                   \
        std::cerr << "Failed to load STG Bullet " << (name) << ": " \
                  << (why) << std::endl;                            \
        return;                                                     \
    }

void ResourceManager::LoadSTGBullet(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif
    int balance_top = lua_gettop(L_main);

    /* Repeat Check */
    if (stg_bullets.contains(name))
        return;

    if (luaL_dofile(L_main, (path_to_stg_bullet + name + ".lua").c_str()))
        INVALID_BULLET(name, lua_tostring(L_main, -1), balance_top);

    /* Lua will return a table. */
    if (!lua_istable(L_main, -1))
        INVALID_BULLET(name, "Lua return not valid!", balance_top);

    STGBulletSetting bs;
    bs.CodeName = name;
    bs.Phy = load_phyfix(name);
    bs.Phy.FD.friction = 0.f;
    bs.Texs = load_stg_texture(name);

    /* Bullet function */
    lua_getfield(L_main, -1, "change");
    int good = luaNOERROR_checkoption(L_main, -1, "JUST_HURT", STG_STATE_CHANGE_CODE);
    if (good < 0)
        INVALID_BULLET(name, "invalid STG change!", balance_top);
    bs.Change.Code = static_cast<STGStateChangeCode>(good);
    lua_pop(L_main, 1);
    lua_getfield(L_main, -1, "damage");
    if (!lua_isinteger(L_main, -1))
        INVALID_BULLET(name, "invalid damage!", balance_top);
    bs.Change.any.Damage = lua_tointeger(L_main, -1);
    lua_pop(L_main, 1);

    /* Bullet patterns */
    bs.KS = load_kinematic_seq(name);

    lua_pop(L_main, 1);
    stg_bullets.emplace(name, std::move(bs));

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif
}

STGBulletSetting &ResourceManager::GetSTGBullet(const std::string &name)
{
    return stg_bullets.at(name);
}

#define INVALID_SHOOTER(name, why, un)                               \
    {                                                                \
        lua_settop(L_main, (un));                                    \
        std::cerr << "Failed to load STG Shooter " << (name) << ": " \
                  << (why) << std::endl;                             \
        return;                                                      \
    }

void ResourceManager::LoadSTGShooter(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif
    int balance_top = lua_gettop(L_main);

    /* Repeat Check */
    if (stg_shooters.contains(name))
        return;

    if (luaL_dofile(L_main, (path_to_stg_shooter + name + ".lua").c_str()))
        INVALID_SHOOTER(name, lua_tostring(L_main, -1), balance_top);

    /* Lua will return a table. */
    if (!lua_istable(L_main, -1))
        INVALID_SHOOTER(name, "Lua return not valid!", balance_top);

    STGShooterSetting ss;
    ss.CodeName = name;

    /* stg para */
    lua_getfield(L_main, -1, "power");
    if (!lua_isinteger(L_main, -1))
        INVALID_SHOOTER(name, "invalid damage!", balance_top);
    ss.Power = lua_tointeger(L_main, -1);
    lua_pop(L_main, 1);
    if (lua_getfield(L_main, -1, "speed") != LUA_TNUMBER)
        INVALID_SHOOTER(name, "invalid speed!", balance_top);
    ss.Speed = lua_tonumber(L_main, -1);
    lua_pop(L_main, 1);
    if (lua_getfield(L_main, -1, "ammo_slots_n") != LUA_TNUMBER)
        INVALID_SHOOTER(name, "invalid ammo slots number!", balance_top);
    ss.AmmoSlotsNum = lua_tonumber(L_main, -1);
    lua_pop(L_main, 1);

    /* patterns */
    lua_getfield(L_main, -1, "pattern");
    int good = luaNOERROR_checkoption(L_main, -1, nullptr, SS_PATTERNS_CODE);
    if (good < 0)
        INVALID_SHOOTER(name, "invalid pattern", balance_top);
    ss.Pattern = static_cast<SSPatternsCode>(good);
    lua_pop(L_main, 1);

    switch (ss.Pattern)
    {
    case SSPatternsCode::CONTROLLED:
        if (lua_getfield(L_main, -1, "data") != LUA_TFUNCTION)
            INVALID_SHOOTER(name, "invalid update function!", balance_top)
        lua_getfield(L_main, LUA_REGISTRYINDEX, STG_SHOOT_FUNCTIONS_KEY);
        lua_pushvalue(L_main, -2);
        lua_setfield(L_main, -2, name.c_str());
        lua_pop(L_main, 2);
        break;

    case SSPatternsCode::TOTAL_TURN:
        if (lua_getfield(L_main, -1, "data") != LUA_TNUMBER)
            INVALID_SHOOTER(name, "invalid total turn speed!", balance_top)
        ss.Data.turn_speed = lua_tonumber(L_main, -1) / UPDATE_PER_SEC;
        lua_pop(L_main, 1);
        break;

    case SSPatternsCode::SPLIT_TURN:
        if (lua_getfield(L_main, -1, "data") != LUA_TTABLE || luaL_len(L_main, -1) > MAX_LUNCHERS_NUM)
            INVALID_SHOOTER(name, "invalid split turn speeds!", balance_top)
        for (int i = 0; i < luaL_len(L_main, -1); i++)
        {
            if (lua_geti(L_main, -1, i + 1) != LUA_TNUMBER)
                INVALID_SHOOTER(name, "invalid split turn speeds!", balance_top);
            ss.Data.turn_speeds[i] = lua_tonumber(L_main, -1) / UPDATE_PER_SEC;
            lua_pop(L_main, 1);
        }
        lua_pop(L_main, 1);
        break;

    default:
        ss.Pattern = SSPatternsCode::STAY;
        break;
    }

    /* lunchers */
    if (lua_getfield(L_main, -1, "lunchers") != LUA_TTABLE || luaL_len(L_main, -1) > MAX_LUNCHERS_NUM)
        INVALID_SHOOTER(name, "invalid lunchers", balance_top);
    for (int i = 0; i < luaL_len(L_main, -1); i++)
    {
        if (lua_geti(L_main, -1, i + 1) != LUA_TTABLE)
            INVALID_SHOOTER(name, "invalid lunchers", balance_top);

        if (lua_getfield(L_main, -1, "d_pos") != LUA_TTABLE)
            INVALID_SHOOTER(name, "invalid lunchers", balance_top);
        if (lua_geti(L_main, -1, 1) != LUA_TNUMBER || lua_geti(L_main, -2, 2) != LUA_TNUMBER)
            INVALID_SHOOTER(name, "invalid lunchers' d_pos", balance_top);
        ss.Lunchers[i].DAttitude.Pos.x = lua_tonumber(L_main, -2);
        ss.Lunchers[i].DAttitude.Pos.y = lua_tonumber(L_main, -1);
        lua_pop(L_main, 3);

        if (lua_getfield(L_main, -1, "d_angle") != LUA_TNUMBER)
            INVALID_SHOOTER(name, "invalid lunchers' d_angle", balance_top);
        ss.Lunchers[i].DAttitude.Angle = lua_tonumber(L_main, -1);
        lua_pop(L_main, 1);

        if (lua_getfield(L_main, -1, "interval") != LUA_TNUMBER)
            INVALID_SHOOTER(name, "invalid lunchers' interval", balance_top);
        ss.Lunchers[i].Interval = std::lroundf(lua_tonumber(L_main, -1) * static_cast<float>(UPDATE_PER_SEC));
        lua_pop(L_main, 1);

        if (lua_getfield(L_main, -1, "ammo_slot") != LUA_TNUMBER || !lua_isinteger(L_main, -1))
            INVALID_SHOOTER(name, "invalid lunchers' ammo_slot", balance_top);
        ss.Lunchers[i].AmmoSlot = lua_tointeger(L_main, -1) - 1;
        lua_pop(L_main, 1);

        lua_pop(L_main, 1);
    }
    ss.LuncherSize = luaL_len(L_main, -1);
    lua_pop(L_main, 1);

    lua_pop(L_main, 1);
    stg_shooters.emplace(name, std::move(ss));

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif
}

STGShooterSetting &ResourceManager::GetSTGShooter(const std::string &name)
{
    return stg_shooters.at(name);
}

void ResourceManager::LoadSave()
{
    // For now ...
    LoadSTGChar("test_player");
    LoadSTGShooter("test_shooter");
    LoadSTGShooter("test_shooter_slow");
}

void ResourceManager::LoadFont()
{
    std::vector<std::pair<std::string, std::string>> font_file = {
        {"art/SourceHanSansCN-Regular.otf", "source"},
        {"art/PixelMplus12-Regular.ttf", "m+12r"},
        {"art/PixelMplus10-Regular.ttf", "m+10r"},
        {"art/PixelMplus12-Bold.ttf", "m+12b"},
        {"art/PixelMplus10-Bold.ttf", "m+10b"}};
    std::vector<int> size = {10, 12, 20, 24, 30, 36};

    for (auto e : font_file)
        for (auto ee : size)
        {
            ALLEGRO_FONT *font = al_load_font(e.first.c_str(), ee, 0);
            if (!font)
                std::cerr << "Fail to load font: " << e.first << ee << "!\n";
            else
                fonts.emplace(e.second + "_" + std::to_string(ee), font);
        }
}

ALLEGRO_FONT *ResourceManager::GetFont(const std::string &name)
{
    return fonts.at(name);
}

#define INVALID_PHYSICS(name, why, un)                               \
    {                                                                \
        lua_settop(L_main, (un));                                    \
        std::cerr << "Failed to load fixture for " << (name) << ": " \
                  << (why) << std::endl;                             \
        return PhysicalFixture();                                    \
    }

PhysicalFixture ResourceManager::load_phyfix(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif
    int balance_top = lua_gettop(L_main);

    PhysicalFixture pf;

    pf.Physical = false;
    lua_getfield(L_main, -1, "physical");
    if (lua_istable(L_main, -1))
    {
        pf.Physical = true;

        /* Mass */
        lua_getfield(L_main, -1, "density");
        if (!(lua_isnumber(L_main, -1) || lua_isnil(L_main, -1)))
            INVALID_PHYSICS(name, "invalid density!", balance_top);
        pf.FD.density = lua_tonumber(L_main, -1);
        lua_pop(L_main, 1);

        /* Get shape */
        lua_getfield(L_main, -1, "shape");
        int good = luaNOERROR_checkoption(L_main, -1, nullptr, SHAPE_TYPE);
        if (good < 0)
            INVALID_PHYSICS(name, "invalid shape!", balance_top);
        pf.Shape = static_cast<ShapeType>(good);
        lua_pop(L_main, 1);
        if (lua_getfield(L_main, -1, "pos") != LUA_TTABLE ||
            lua_geti(L_main, -1, 1) != LUA_TNUMBER || lua_geti(L_main, -2, 2) != LUA_TNUMBER)
            INVALID_PHYSICS(name, "invalid pos!", balance_top);
        float x = lua_tonumber(L_main, -2);
        float y = lua_tonumber(L_main, -1);
        lua_pop(L_main, 3);

        if (lua_getfield(L_main, -1, "size") != LUA_TTABLE)
            INVALID_PHYSICS(name, "invalid size!", balance_top);
        switch (pf.Shape)
        {
        case ShapeType::CIRCLE:
            if (lua_geti(L_main, -1, 1) != LUA_TNUMBER)
                INVALID_PHYSICS(name, "invalid size!", balance_top);
            pf.C.m_p.Set(x, y);
            pf.C.m_radius = lua_tonumber(L_main, -1);
            lua_pop(L_main, 2);
            break;
        case ShapeType::BOX:
            if (lua_geti(L_main, -1, 1) != LUA_TNUMBER || lua_geti(L_main, -2, 2) != LUA_TNUMBER)
                INVALID_PHYSICS(name, "invalid size!", balance_top);
            pf.P.SetAsBox(lua_tonumber(L_main, -2), lua_tonumber(L_main, -1), b2Vec2(x, y), 0.f);
            lua_pop(L_main, 3);
            break;
        default:
            INVALID_PHYSICS(name, "invalid physical shape!", balance_top);
        }
    }

    lua_pop(L_main, 1);

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif

    return pf;
}

STGTexture ResourceManager::load_stg_texture(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif

    STGTexture st;

    /* Texture things */
    if (lua_getfield(L_main, -1, "sprites") != LUA_TSTRING)
    {
        lua_pop(L_main, 1);
        st.VeryFirstTex = nullptr;
        st.SpriteBornType = SpriteType::NONE;
        st.SpriteShootingType = SpriteType::NONE;
        st.SpriteShiftType = SpriteType::NONE;
        st.SpriteSyncType = SpriteType::NONE;
        st.SpriteFSyncType = SpriteType::NONE;
        st.SpriteHitType = SpriteType::NONE;
        st.SpriteDisableType = SpriteType::NONE;
        st.SpriteMovementType = SpriteType::NONE;
        return st;
    }
    std::string art_name = std::string(lua_tostring(L_main, -1));
    LoadSpriteSheet(art_name);
    lua_pop(L_main, 1);

    /* texture res */
    st.SpriteBorn = art_name + "_born";
    if (textures.contains(st.SpriteBorn))
        st.SpriteBornType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteBorn))
        st.SpriteBornType = SpriteType::ANIMED;
    else
        st.SpriteBornType = SpriteType::NONE;
    st.SpriteShooting = art_name + "_shooting";
    if (textures.contains(st.SpriteShooting))
        st.SpriteShootingType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteShooting))
        st.SpriteShootingType = SpriteType::ANIMED;
    else
        st.SpriteShootingType = SpriteType::NONE;
    st.SpriteShift = art_name + "_shift";
    if (textures.contains(st.SpriteShift))
        st.SpriteShiftType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteShift))
        st.SpriteShiftType = SpriteType::ANIMED;
    else
        st.SpriteShiftType = SpriteType::NONE;
    st.SpriteSync = art_name + "_sync";
    if (textures.contains(st.SpriteSync))
        st.SpriteSyncType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteSync))
        st.SpriteSyncType = SpriteType::ANIMED;
    else
        st.SpriteSyncType = SpriteType::NONE;
    st.SpriteFSync = art_name + "_fsync";
    if (textures.contains(st.SpriteFSync))
        st.SpriteFSyncType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteFSync))
        st.SpriteFSyncType = SpriteType::ANIMED;
    else
        st.SpriteFSyncType = SpriteType::NONE;
    st.SpriteHit = art_name + "_hit";
    if (textures.contains(st.SpriteHit))
        st.SpriteHitType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteHit))
        st.SpriteHitType = SpriteType::ANIMED;
    else
        st.SpriteHitType = SpriteType::NONE;
    st.SpriteDisable = art_name + "disable";
    if (textures.contains(st.SpriteDisable))
        st.SpriteDisableType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteDisable))
        st.SpriteDisableType = SpriteType::ANIMED;
    else
        st.SpriteDisableType = SpriteType::NONE;
    st.SpriteMovement[static_cast<int>(Movement::IDLE)] = art_name + "_idle";
    st.SpriteMovement[static_cast<int>(Movement::UP)] = art_name + "_up";
    st.SpriteMovement[static_cast<int>(Movement::DOWN)] = art_name + "_down";
    st.SpriteMovement[static_cast<int>(Movement::LEFT)] = art_name + "_left";
    st.SpriteMovement[static_cast<int>(Movement::RIGHT)] = art_name + "_right";
    st.SpriteMovement[static_cast<int>(Movement::UL)] = art_name + "_ul";
    st.SpriteMovement[static_cast<int>(Movement::UR)] = art_name + "_ur";
    st.SpriteMovement[static_cast<int>(Movement::DL)] = art_name + "_dl";
    st.SpriteMovement[static_cast<int>(Movement::DR)] = art_name + "_dr";
    if (textures.contains(st.SpriteMovement[static_cast<int>(Movement::IDLE)]))
        st.SpriteMovementType = SpriteType::STATIC;
    else if (animations.contains(st.SpriteMovement[static_cast<int>(Movement::IDLE)]))
        st.SpriteMovementType = SpriteType::ANIMED;
    else
    {
        std::cerr << "STG texture for " << name << " contains no idle sprite!\n";
        return st;
    }

    /* very first texture to init renderer */
    Anime ta;
    switch (st.SpriteBornType)
    {
    case SpriteType::STATIC:
        st.VeryFirstTex = GetTexture(st.SpriteBorn);
        break;
    case SpriteType::ANIMED:
        ta = GetAnime(st.SpriteBorn);
        ta.Forward();
        st.VeryFirstTex = ta.Playing;
        break;
    default:
        switch (st.SpriteMovementType)
        {
        case SpriteType::STATIC:
            st.VeryFirstTex = GetTexture(st.SpriteMovement[static_cast<int>(Movement::IDLE)]);
            break;
        case SpriteType::ANIMED:
            ta = GetAnime(st.SpriteMovement[static_cast<int>(Movement::IDLE)]);
            ta.Forward();
            st.VeryFirstTex = ta.Playing;
            break;
        }
        break;
    }

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif

    return st;
}

#define INVALID_KINEMATIC_PHASES(name, why, un)                           \
    {                                                                     \
        lua_settop(L_main, (un));                                         \
        std::cerr << "Failed to load kinematic phases " << (name) << ": " \
                  << (why) << std::endl;                                  \
        return KinematicSeq();                                            \
    }

KinematicSeq ResourceManager::load_kinematic_seq(const std::string &name)
{
#ifdef _DEBUG
    int _d_top = lua_gettop(L_main);
#endif
    int balance_top = lua_gettop(L_main);

    KinematicSeq kps;

    if (lua_getfield(L_main, -1, "kinematic_seq") != LUA_TTABLE)
        INVALID_KINEMATIC_PHASES(name, "invalid ks table!", balance_top);

    /* track? */
    if (lua_getfield(L_main, -1, "track") != LUA_TBOOLEAN)
        INVALID_KINEMATIC_PHASES(name, "invalid track!", balance_top);
    kps.Track = lua_toboolean(L_main, -1);
    lua_pop(L_main, 1);

    /* LOOP? */
    if (lua_getfield(L_main, -1, "loop") != LUA_TBOOLEAN)
        INVALID_KINEMATIC_PHASES(name, "invalid loop!", balance_top);
    kps.Loop = lua_toboolean(L_main, -1);
    lua_pop(L_main, 1);

    /* Always go front? */
    if (lua_getfield(L_main, -1, "dir") != LUA_TBOOLEAN)
        INVALID_KINEMATIC_PHASES(name, "invalid dir!", balance_top);
    kps.Dir = lua_toboolean(L_main, -1);
    lua_pop(L_main, 1);

    /* Speed change. */
    if (lua_getfield(L_main, -1, "seq") != LUA_TTABLE || luaL_len(L_main, -1) > MAX_KINEMATIC_PHASE_NUM)
        INVALID_KINEMATIC_PHASES(name, "invalid speed seq!", balance_top);
    for (int i = 0; i < luaL_len(L_main, -1); i++)
    {
        if (lua_geti(L_main, -1, i + 1) != LUA_TTABLE)
            INVALID_KINEMATIC_PHASES(name, "invalid speed seq!", balance_top);

        if (lua_geti(L_main, -1, 1) != LUA_TNUMBER)
            INVALID_KINEMATIC_PHASES(name, "invalid speed Vv!", balance_top);
        if (lua_geti(L_main, -2, 2) != LUA_TNUMBER)
            INVALID_KINEMATIC_PHASES(name, "invalid speed Va!", balance_top);
        if (lua_geti(L_main, -3, 3) != LUA_TNUMBER)
            INVALID_KINEMATIC_PHASES(name, "invalid speed time!", balance_top);
        if (lua_geti(L_main, -4, 4) != LUA_TNUMBER)
            INVALID_KINEMATIC_PHASES(name, "invalid speed dur!", balance_top);

        kps.Seq[i] = {static_cast<float>(lua_tonumber(L_main, -4)),
                      static_cast<float>(lua_tonumber(L_main, -3)),
                      std::lroundf(lua_tonumber(L_main, -2) * static_cast<float>(UPDATE_PER_SEC)),
                      static_cast<float>(lua_tonumber(L_main, -1))};
        kps.Seq[i].TransEnd = kps.Seq[i].PhaseTime + (kps.Seq[i].TransTime * static_cast<float>(UPDATE_PER_SEC));
        lua_pop(L_main, 5);
    }
    kps.SeqSize = luaL_len(L_main, -1);
    lua_pop(L_main, 1);

    lua_pop(L_main, 1);

#ifdef _DEBUG
    if (lua_gettop(L_main) != _d_top)
        luaL_error(L_main, "unbalance stack");
#endif

    return kps;
}
