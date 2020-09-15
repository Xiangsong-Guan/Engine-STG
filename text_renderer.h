#ifndef GAME_TEXT_RENDER_H
#define GAME_TEXT_RENDER_H

#include "data_struct.h"

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>

#include <string>

class TextRenderer
{
private:
    float x, y;
    TextItem text;

public:
    ALLEGRO_COLOR Color;

    TextRenderer();
    TextRenderer(const TextRenderer &) = delete;
    TextRenderer(TextRenderer &&) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;
    TextRenderer &operator=(TextRenderer &&) = delete;
    ~TextRenderer() = default;

    void Draw() const;
    void SetText(TextItem t) noexcept;
    const std::string &GetContent() const noexcept;

    void ChangeText(float f, int w);
    void ChangeText(int i);

    float GetWidth() const;
    float GetRight() const;
};

#endif