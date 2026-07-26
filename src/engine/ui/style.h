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

    constexpr bool isZero() const {
        return left.isAuto() && top.isAuto() &&
               right.isAuto() && bottom.isAuto();
    }
};

enum class Display {
    None,  // children use absolute inset layout (default)
    Block, // relative children stack vertically
    Flex,  // relative children flow along flexDirection
};

enum class FlexDirection {
    Row,
    Column,
};

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
};

enum class AlignItems {
    Start,
    Center,
    End,
    Stretch,
};

enum class PositionMode {
    Absolute,
    Relative,
};

enum class TextOverflow {
    Visible,
    Clip,
    Wrap,
};

struct Style {
    Display display = Display::None;
    PositionMode position = PositionMode::Absolute;
    FlexDirection flexDirection = FlexDirection::Row;
    JustifyContent justifyContent = JustifyContent::Start;
    AlignItems alignItems = AlignItems::Stretch;

    Insets inset;
    Insets margin;
    Insets padding;

    Length width;
    Length height;
    Length minWidth;
    Length minHeight;
    Length maxWidth;
    Length maxHeight;

    // Space between consecutive flow children along the main axis.
    Length gap;

    // Flex item growth on the main axis (flex containers only).
    float flexGrow = 0.f;
    Length flexBasis;

    TextOverflow textOverflow = TextOverflow::Clip;

    // Absolute placement via inset and/or explicit size. Position mode alone
    // does not opt in — relative children are placed by block flow instead.
    bool hasAbsolutePlacement() const {
        return display != Display::None ||
               !inset.left.isAuto() || !inset.top.isAuto() ||
               !inset.right.isAuto() || !inset.bottom.isAuto() ||
               !width.isAuto() || !height.isAuto() ||
               !minWidth.isAuto() || !minHeight.isAuto() ||
               !maxWidth.isAuto() || !maxHeight.isAuto();
    }
};
