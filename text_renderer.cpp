#include "text_renderer.h"

TextRenderer::TextRenderer() : Color(al_map_rgb_f(1.f, 1.f, 1.f)) {}

void TextRenderer::Draw() const
{
    al_draw_text(text.Font, Color, x, y, text.Align, text.Text.c_str());
}

void TextRenderer::SetText(TextItem t) noexcept
{
    text = std::move(t);
    x = SCREEN_WIDTH * t.X;
    y = SCREEN_HEIGHT * t.Y;
}

void TextRenderer::ChangeText(float f, int w)
{
    text.Text = std::to_string(f);
    text.Text.resize(w);
}

void TextRenderer::ChangeText(int i)
{
    text.Text = std::to_string(i);
}

float TextRenderer::GetWidth() const
{
    return static_cast<float>(al_get_text_width(text.Font, text.Text.c_str())) / static_cast<float>(SCREEN_WIDTH);
}

float TextRenderer::GetRight() const
{
    return (x + static_cast<float>(al_get_text_width(text.Font, text.Text.c_str()))) / static_cast<float>(SCREEN_WIDTH);
}