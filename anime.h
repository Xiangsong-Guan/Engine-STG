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
    int duration;

public:
    ALLEGRO_BITMAP *Playing;

    Anime() : now_(-1), sync_(-1), duration(0), Playing(nullptr) {}
    Anime(const Anime &) = default;
    Anime(Anime &&) = default;
    Anime &operator=(const Anime &) = default;
    Anime &operator=(Anime &&) = default;
    ~Anime() = default;

    Anime(std::vector<ALLEGRO_BITMAP *> &&frames) : frames_(std::move(frames)),
                                                    now_(-1),
                                                    sync_(-1),
                                                    duration(static_cast<int>(frames_.size())),
                                                    Playing(nullptr) {}

    inline bool Forward() noexcept
    {
        sync_ = (sync_ + 1) % ANIME_UPDATE_TIMER;
        if (sync_ != 0)
            return false;

        now_ = (now_ + 1) % duration;
        Playing = frames_[now_];
        return true;
    }

    inline void Reset() noexcept
    {
        now_ = -1;
        sync_ = -1;
        Playing = nullptr;
    }

    inline int Duration() const noexcept
    {
        return duration * ANIME_UPDATE_TIMER;
    }
};

#endif
