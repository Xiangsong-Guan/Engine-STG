#include "text_renderer.h"

#include "cppsuckdef.h"

TextRenderer::TextRenderer() : Color(al_map_rgb_f(1.f, 1.f, 1.f)) {}

void TextRenderer::Draw() const
{
    al_draw_text(Text.Font, Color, Text.X, Text.Y, Text.Align, Text.Text.c_str());
}

void TextRenderer::ChangeText(float f, int w)
{
    Text.Text = std::to_string(f);
    Text.Text.resize(w);
}

void TextRenderer::ChangeText(int i)
{
    Text.Text = std::to_string(i);
}

float TextRenderer::GetWidth() const
{
    return static_cast<float>(al_get_text_width(Text.Font, Text.Text.c_str()));
}

float TextRenderer::GetRight() const
{
    return Text.X + static_cast<float>(al_get_text_width(Text.Font, Text.Text.c_str()));
}

float TextRenderer::GetHeight() const
{
    return static_cast<float>(al_get_font_line_height(Text.Font));
}

float TextRenderer::GetBottom() const
{
    return Text.Y + static_cast<float>(al_get_font_line_height(Text.Font));
}