#include "text_renderer.h"

TextRenderer::TextRenderer() : Color(al_map_rgb_f(1.f, 1.f, 1.f)) {}

void TextRenderer::Draw() const
{
    al_draw_text(text.Font, Color, x, y, text.Align,
                 reinterpret_cast<const char *>(text.Text.c_str()));
}

void TextRenderer::SetWH(int w, int h) noexcept
{
    width = w;
    height = h;
    x = w * text.X;
    y = h * text.Y;
}

void TextRenderer::SetText(TextItem t) noexcept
{
    text = std::move(t);
    x = width * t.X;
    y = height * t.Y;
}