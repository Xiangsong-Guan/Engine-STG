#ifndef ANIME_H
#define ANIME_H

#include "cppsuckdef.h"

#include <allegro5/allegro5.h>

#include <vector>

class Anime
{
private:
    std::vector<ALLEGRO_BITMAP *> frames_;
    int now_;
    int sync_;

public:
    /* Duration in anime */
    int DURATION;
    /* Duration in logic */
    int LG_DURATION;
    ALLEGRO_BITMAP *Playing;

    Anime() : now_(-1), sync_(-1), DURATION(0), LG_DURATION(0), Playing(nullptr) {}
    Anime(const Anime &) = default;
    Anime(Anime &&) = default;
    Anime &operator=(const Anime &) = default;
    Anime &operator=(Anime &&) = default;
    ~Anime() = default;

    Anime(std::vector<ALLEGRO_BITMAP *> &&frames) : frames_(std::move(frames)),
                                                    now_(-1),
                                                    sync_(-1),
                                                    DURATION(static_cast<int>(frames_.size())),
                                                    LG_DURATION(DURATION * ANIME_UPDATE_TIMER),
                                                    Playing(nullptr) {}
    Anime(ALLEGRO_BITMAP *one_frame) : frames_(std::vector{one_frame}),
                                       now_(-1),
                                       sync_(-1),
                                       DURATION(1),
                                       LG_DURATION(1 * ANIME_UPDATE_TIMER),
                                       Playing(nullptr) {}

    inline bool Forward() noexcept
    {
        sync_ = (sync_ + 1) % ANIME_UPDATE_TIMER;
        if (sync_ != 0)
            return false;

        now_ = (now_ + 1) % DURATION;
        if (frames_[now_] == Playing)
            return false;

        Playing = frames_[now_];
        return true;
    }

    inline void Reset() noexcept
    {
        now_ = -1;
        sync_ = -1;
        Playing = nullptr;
    }

    inline ALLEGRO_BITMAP *GetFrame(int timer) noexcept
    {
        return frames_[(timer / ANIME_UPDATE_TIMER) % DURATION];
    }
};

#endif
