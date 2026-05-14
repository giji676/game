#ifndef GJ_APP_H
#define GJ_APP_H

#include <SDL2/SDL_video.h>

enum class WindowMode {
    Windowed,
    BorderlessFullscreen,
    ExclusiveFullscreen
};

struct Resolution {
    int width;
    int height;

    constexpr float aspectRatio() const {
        return static_cast<float>(width) / height;
    }

    bool operator==(const Resolution& other) const {
        return width == other.width && height == other.height;
    }
};

namespace PresetResolutions {
    constexpr Resolution R1280x720  {1280, 720};
    constexpr Resolution R1600x900  {1600, 900};
    constexpr Resolution R1920x1080 {1920, 1080};
    constexpr Resolution R2560x1440 {2560, 1440};
    constexpr Resolution R3840x2160 {3840, 2160};
}

enum class AntiAliasing {
    None = 0,
    MSAA2X = 2,
    MSAA4X = 4,
    MSAA8X = 8
};

struct VideoSettings {
    Resolution resolution = PresetResolutions::R1920x1080;
    WindowMode mode = WindowMode::Windowed;
    AntiAliasing antiAliasing = AntiAliasing::MSAA2X;
    bool vsync = true;
};

class GJ_App {
public:
    SDL_Window *window;
    SDL_GLContext glContext;
    VideoSettings videoSettings;

    float deltaTime = 0.0f;
    float lastFrameTime = 0.0f;

    int initialize();
    void toggleWindow();

    int width() const;
    int height() const;
    float aspectRatio() const { return videoSettings.resolution.aspectRatio(); }
    int antiAliasing() const { return static_cast<int>(videoSettings.antiAliasing); }
    float getDeltaTime();

private:
    Uint32 getWindowFlags() const;
    VideoSettings prevVideoSettings;
    void updateViewport();
};

#endif
