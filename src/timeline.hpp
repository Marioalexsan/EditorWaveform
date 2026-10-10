#pragma once

#include <Geode/Geode.hpp>
#include "waveform.hpp"
#include "config.hpp"
#include <filesystem>

namespace waveform {
class Timeline final : public cocos2d::CCNode {
    LevelEditorLayer* m_editor = nullptr;
    geode::Ref<cocos2d::CCDrawNode> m_overlay;
    SourceLoader m_source;
    std::string m_path;
    std::filesystem::file_time_type m_modified{};
    float m_songOffset = 0;
    float m_pollTimer = 0;
    bool m_enabled = false;
    bool m_suspended = false;
    float m_worldBaseline = 0;

    bool init(LevelEditorLayer* editor);
    void pollSong();
    void renderGrid(Config const& config);
public:
    static Timeline* create(LevelEditorLayer* editor);
    void update(float dt) override;
    void onExit() override;
    void onEnter() override;
};
}
