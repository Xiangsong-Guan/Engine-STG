#ifndef GAME_SCENE_H
#define GAME_SCENE_H

class Scene
{
public:
    Scene() = default;
    Scene(const Scene &) = delete;
    Scene(Scene &&) = delete;
    Scene &operator=(const Scene &) = delete;
    Scene &operator=(Scene &&) = delete;
    virtual ~Scene() = default;

    virtual void Update() = 0;
    virtual void Render(float forward_time) = 0;
};

#endif