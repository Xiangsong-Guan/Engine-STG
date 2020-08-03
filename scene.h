#ifndef GAME_SCENE_H
#define GAME_SCENE_H

class Scene
{
public:
    virtual void Update() = 0;
    virtual void Render(float forward_time) = 0;
};

#endif