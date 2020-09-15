#ifndef CPPSUCKDEF_H
#define CPPSUCKDEF_H

// #define STG_JUST_DRAW_PHY
// #define STG_DEBUG_PHY_DRAW
#define STG_PERFORMENCE_SHOW
#define STG_LUA_API_ARG_CHECK

constexpr int UPDATE_PER_SEC = 60;
/* 逻辑帧时间 */
constexpr float SEC_PER_UPDATE = 1.f / static_cast<float>(UPDATE_PER_SEC);
constexpr float MS_PER_UPDATE = 1000.f / static_cast<float>(UPDATE_PER_SEC);
constexpr float SEC_PER_UPDATE_BY2_SQ = (SEC_PER_UPDATE * SEC_PER_UPDATE) / 4.f;
/* 原生分辨率 宽 */
constexpr int SCREEN_WIDTH = 480;
/* 原生分辨率 高 */
constexpr int SCREEN_HEIGHT = 360;
/* 物理世界到渲染像素区的缩放 */
constexpr float PIXIL_PRE_M = 32.f / 1.6f;
constexpr float M_PRE_PIXIL = 1.6f / 32.f;
/* 物理世界的边界 */
constexpr float PHYSICAL_WIDTH = SCREEN_WIDTH * M_PRE_PIXIL;
constexpr float PHYSICAL_HEIGHT = SCREEN_HEIGHT * M_PRE_PIXIL;
constexpr float STG_FIELD_BOUND_BUFFER = 3.f;

/* 动画帧长 */
constexpr int MS_PRE_ANIME_FRAME = 50;
/* 动画帧与逻辑帧更新的比例 */
constexpr int ANIME_UPDATE_TIMER = MS_PRE_ANIME_FRAME * UPDATE_PER_SEC / 1000;

/* Physics sim setting */
constexpr int VELOCITY_ITERATIONS = 6;
constexpr int POSITION_ITERATIONS = 2;

/* Limit */
constexpr int MAX_ENTITIES = 256;
constexpr int MAX_ON_STAGE = 64;

/* G */
extern float g_scale;
extern float g_offset_x;
extern float g_offset_y;
extern float g_width;
extern float g_height;
extern float g_time_step;

#endif