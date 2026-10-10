#include <Geode/Geode.hpp>
#include <Geode/modify/LevelEditorLayer.hpp>
#include <Geode/loader/ModEvent.hpp>
#include <eclipse.hpp>
#include <limits>
#include "timeline.hpp"

using namespace geode::prelude;

class $modify(WaveformEditor, LevelEditorLayer) {
    bool init(GJGameLevel* level, bool noUI) {
        if (!LevelEditorLayer::init(level, noUI)) return false;
        if (!noUI && m_editorUI && m_drawGridLayer) {
            if (auto timeline = waveform::Timeline::create(this)) addChild(timeline);
        }
        return true;
    }
};

namespace {
eclipse::components::InputFloat addNumberInput(eclipse::MenuTab const& tab, std::string id,
    std::string title, Function<void(float)> callback, bool reversedArguments) {
    if (!reversedArguments) return tab.addInputFloat(std::move(id), std::move(title), std::move(callback));
    // Eclipse 1.9.4's implementation takes (title, id), opposite to its public
    // helper. Create through the same API, keeping the handle's real config ID.
    auto const& api = eclipse::__internal__::getVTable();
    auto uid = api.CreateInputFloat ? api.CreateInputFloat(tab.getName(), std::move(title), id, std::move(callback)) : 0;
    return eclipse::components::InputFloat(uid, std::move(id));
}
void syncNumber(std::string const& id, double value) {
    if (eclipse::config::get<double>(id, std::numeric_limits<double>::quiet_NaN()) != value) {
        eclipse::config::set(id, value);
    }
}
void registerEclipse() {
    static bool registered = false;
    auto eclipseMod = Loader::get()->getLoadedMod("eclipse.eclipse-menu");
    if (registered || !eclipseMod) return;
    registered = true;
    auto tab = eclipse::MenuTab::find("Editor Waveform");
    auto mod = Mod::get();
    bool reversedArguments = eclipseMod->getVersion() == VersionInfo(1, 9, 4);
    for (auto key : {"enabled", "lock-to-camera"}) tab.addModSettingToggle(mod->getSetting(key));
    auto number = [tab, mod, reversedArguments](std::string key, float minimum, float maximum, bool integer) {
        auto setting = mod->getSetting(key);
        auto id = std::string(mod->getID()) + "." + key;
        auto input = addNumberInput(tab, id, setting->getDisplayName(), [mod, key, integer, minimum, maximum, id](float value) {
            if (std::isfinite(value)) {
                value = std::clamp(value, minimum, maximum);
                if (integer) {
                    auto rounded = static_cast<int64_t>(std::round(value));
                    if (mod->getSettingValue<int64_t>(key) != rounded) mod->setSettingValue<int64_t>(key, rounded);
                } else if (mod->getSettingValue<double>(key) != double(value)) {
                    mod->setSettingValue<double>(key, value);
                }
            }
            auto stored = integer ? double(mod->getSettingValue<int64_t>(key)) : mod->getSettingValue<double>(key);
            syncNumber(id, stored);
        }, reversedArguments);
        input.setMinValue(minimum).setMaxValue(maximum).setFormat(integer ? "%.0f" : "%.2f")
            .setDescription(setting->getDescription().value_or(""));
        auto value = integer ? double(mod->getSettingValue<int64_t>(key)) : mod->getSettingValue<double>(key);
        syncNumber(id, value);
        if (integer) listenForSettingChanges<int64_t>(key, [id](int64_t value) { syncNumber(id, double(value)); }, mod);
        else listenForSettingChanges<double>(key, [id](double value) { syncNumber(id, value); }, mod);
    };
    number("vertical-offset", -1e6f, 1e6f, false);
    number("vertical-scale", 0, 1e6f, false);
    number("line-width", 1, 10, true);
    number("opacity", 0, 255, true);
}
}
$on_mod(Loaded) { registerEclipse(); }
$execute {
    if (auto eclipseMod = Loader::get()->getInstalledMod("eclipse.eclipse-menu")) {
        ModStateEvent(ModEventType::Loaded, eclipseMod).listen([] { registerEclipse(); }).leak();
    }
}
