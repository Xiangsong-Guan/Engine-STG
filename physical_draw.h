#ifndef STG_PHYSICAL_DRAW_H
#define STG_PHYSICAL_DRAW_H

#include <box2d/box2d.h>
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>

#include <vector>

class PhysicalDraw : public b2Draw
{
private:
    static constexpr float TKN_ON_TOP = 2.f;
    static constexpr float ALP_ON_TOP = .5f;

    float physcale;

public:
    PhysicalDraw() = default;
    PhysicalDraw(const PhysicalDraw &) = delete;
    PhysicalDraw(PhysicalDraw &&) = delete;
    PhysicalDraw &operator=(const PhysicalDraw &) = delete;
    PhysicalDraw &operator=(PhysicalDraw &&) = delete;
    ~PhysicalDraw() = default;

    void Init(float ps)
    {
        this->SetFlags(this->e_shapeBit);
        this->physcale = ps;
    }

    void DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) final
    {
        float v[b2_maxPolygonVertices][2];

        for (int32 i = 0; i < vertexCount; i++)
        {
            v[i][0] = vertices[i].x * physcale;
            v[i][1] = vertices[i].y * physcale;
        }

        al_draw_polygon(reinterpret_cast<const float *>(v), vertexCount, ALLEGRO_LINE_JOIN_BEVEL,
                        al_premul_rgba_f(color.r, color.g, color.g, color.a), 1.f, 0.f);
    }

    void DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) final
    {
        float v[b2_maxPolygonVertices][2];

        for (int32 i = 0; i < vertexCount; i++)
        {
            v[vertexCount - i - 1][0] = vertices[i].x * physcale;
            v[vertexCount - i - 1][1] = vertices[i].y * physcale;
        }

        al_draw_filled_polygon(reinterpret_cast<const float *>(v), vertexCount,
                               al_premul_rgba_f(color.r, color.g, color.g, color.a * ALP_ON_TOP));
        al_draw_polygon(reinterpret_cast<const float *>(v), vertexCount, ALLEGRO_LINE_JOIN_BEVEL,
                        al_premul_rgba_f(color.r, color.g, color.g, color.a), TKN_ON_TOP, 0.f);
    }

    void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) final
    {
        al_draw_circle(center.x * physcale, center.y * physcale, radius * physcale,
                       al_premul_rgba_f(color.r, color.g, color.b, color.a), 1.f);
    }

    void DrawSolidCircle(const b2Vec2 &center, float radius,
                         const b2Vec2 &axis, const b2Color &color) final
    {
        al_draw_filled_circle(center.x * physcale, center.y * physcale, radius * physcale,
                              al_premul_rgba_f(color.r, color.g, color.b, color.a * ALP_ON_TOP));
        al_draw_circle(center.x * physcale, center.y * physcale, radius * physcale,
                       al_premul_rgba_f(color.r, color.g, color.b, color.a), TKN_ON_TOP);

        b2Vec2 p = center + radius * axis;
        al_draw_line(center.x * physcale, center.y * physcale, p.x * physcale, p.y * physcale,
                     al_premul_rgba_f(color.r, color.g, color.b, color.a), TKN_ON_TOP);
    }

    void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color)
    {
        al_draw_line(p1.x * physcale, p1.y * physcale, p2.x * physcale, p2.y * physcale,
                     al_premul_rgba_f(color.r, color.g, color.b, color.a), 1.f);
    }

    void DrawTransform(const b2Transform &xf)
    {
        b2Vec2 p1 = xf.p, p2;

        p2 = p1 + xf.q.GetXAxis();
        al_draw_line(p1.x * physcale, p1.y * physcale, p2.x * physcale, p2.y * physcale,
                     al_map_rgb_f(1.0f, 0.0f, 0.0f), 1.f);

        p2 = p1 + xf.q.GetYAxis();
        al_draw_line(p1.x * physcale, p1.y * physcale, p2.x * physcale, p2.y * physcale,
                     al_map_rgb_f(0.0f, 1.0f, 0.0f), 1.f);
    }

    void DrawPoint(const b2Vec2 &p, float size, const b2Color &color)
    {
        al_draw_filled_circle(p.x * physcale, p.y * physcale, size * physcale,
                              al_premul_rgba_f(color.r, color.g, color.b, color.a));
    }
};

#endif
