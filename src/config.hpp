#pragma once

#include <Geode/loader/Mod.hpp>
#include <algorithm>
#include <cmath>

namespace waveform {
struct Config {
    bool enabled;
    bool lockToCamera;
    float verticalOffset;
    float verticalScale;
    int lineWidth;
    int opacity;

    static Config get() {
        auto mod = geode::Mod::get();
        auto finite = [](double value, float fallback) {
            return std::isfinite(value) ? static_cast<float>(std::clamp(value, -1e6, 1e6)) : fallback;
        };
        return {
            mod->getSettingValue<bool>("enabled"),
            mod->getSettingValue<bool>("lock-to-camera"),
            finite(mod->getSettingValue<double>("vertical-offset"), 270.f),
            std::max(0.f, finite(mod->getSettingValue<double>("vertical-scale"), 90.f)),
            static_cast<int>(std::clamp<int64_t>(mod->getSettingValue<int64_t>("line-width"), 1, 10)),
            static_cast<int>(std::clamp<int64_t>(mod->getSettingValue<int64_t>("opacity"), 0, 255))
        };
    }
};
}
