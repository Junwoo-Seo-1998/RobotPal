#ifndef __GLOBALCOMPONENT_H__
#define __GLOBALCOMPONENT_H__
struct WindowData {
    float width=1.0f, height=1.0f;
    float GetAspect() const { return width / height; }
};

#endif
