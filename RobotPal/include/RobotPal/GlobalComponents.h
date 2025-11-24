struct WindowData {
    float width=1.0f, height=1.0f;
    float GetAspect() const { return width / height; }
};