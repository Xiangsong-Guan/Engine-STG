#ifndef ANIME_H
#define ANIME_H

#include "cppsuckdef.h"
#include "data_struct.h"

#include <allegro5/allegro5.h>

#include <vector>

class Anime
{
private:
    static constexpr SpriteItem NULL_SPRITE_ITEM = {nullptr, 0.f, 0.f};

    std::vector<SpriteItem> frames_;
    int now_;
    int sync_;

public:
    /* Duration in anime */
    int DURATION;
    /* Duration in logic */
    int LG_DURATION;
    const SpriteItem *Playing;

    Anime() : now_(-1), sync_(-1), DURATION(0), LG_DURATION(0), Playing(&NULL_SPRITE_ITEM) {}
    Anime(const Anime &) = default;
    Anime(Anime &&) = default;
    Anime &operator=(const Anime &) = default;
    Anime &operator=(Anime &&) = default;
    ~Anime() = default;

    Anime(std::vector<SpriteItem> &&frames) : frames_(std::move(frames)),
                                              now_(-1),
                                              sync_(-1),
                                              DURATION(static_cast<int>(frames_.size())),
                                              LG_DURATION(DURATION * ANIME_UPDATE_TIMER),
                                              Playing(&NULL_SPRITE_ITEM) {}
    Anime(const SpriteItem &one_frame) : frames_(std::vector{one_frame}),
                                         now_(-1),
                                         sync_(-1),
                                         DURATION(1),
                                         LG_DURATION(1 * ANIME_UPDATE_TIMER),
                                         Playing(&NULL_SPRITE_ITEM) {}

    inline bool Forward() noexcept
    {
        sync_ = (sync_ + 1) % ANIME_UPDATE_TIMER;
        if (sync_ != 0)
            return false;

        now_ = (now_ + 1) % DURATION;
        if (frames_[now_].Sprite == Playing->Sprite)
            return false;

        Playing = frames_.data() + now_;
        return true;
    }

    inline void Reset() noexcept
    {
        now_ = -1;
        sync_ = -1;
        Playing = &NULL_SPRITE_ITEM;
    }

    inline const SpriteItem *GetFrame(int timer) noexcept
    {
        return frames_.data() + ((timer / ANIME_UPDATE_TIMER) % DURATION);
    }
};

#endif
