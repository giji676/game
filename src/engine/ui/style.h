#pragma once

enum class Unit {
    Auto,
    Px,
    Percent
};

struct Length {
    Unit unit = Unit::Auto;
    float value = 0.f;

    static constexpr Length px(float v) { return {Unit::Px, v}; }
    static constexpr Length percent(float v) { return {Unit::Percent, v}; }
    static constexpr Length automatic() { return {}; }

    constexpr bool isAuto() const { return unit == Unit::Auto; }

    constexpr float resolve(float basis) const {
        switch (unit) {
            case Unit::Px:      return value;
            case Unit::Percent: return basis * value * 0.01f;
            case Unit::Auto:
            default:            return 0.f;
        }
    }
};

// Distance from each edge of the parent box, as in CSS `inset`.
struct Insets {
    Length left;
    Length top;
    Length right;
    Length bottom;
};

struct Style {
    Insets inset;
    Length width;
    Length height;

    // Elements that specify nothing keep the transform they were given, so
    // manually placed UI keeps working alongside styled UI.
    bool specifiesLayout() const {
        return !inset.left.isAuto() || !inset.top.isAuto() ||
               !inset.right.isAuto() || !inset.bottom.isAuto() ||
               !width.isAuto() || !height.isAuto();
    }
};
