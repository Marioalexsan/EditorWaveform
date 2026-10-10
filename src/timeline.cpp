#include "timeline.hpp"

using namespace geode::prelude;

namespace waveform {
namespace {
void rectangle(CCDrawNode* node, float x1, float y1, float x2, float y2, ccColor4F color) {
    if (!std::isfinite(x1) || !std::isfinite(y1) || !std::isfinite(x2) || !std::isfinite(y2)) return;
    CCPoint points[] = {{x1, y1}, {x2, y1}, {x2, y2}, {x1, y2}};
    node->drawPolygon(points, 4, color, 0, {0, 0, 0, 0});
}
}
Timeline* Timeline::create(LevelEditorLayer* editor) {
    auto result = new Timeline;
    if (result->init(editor)) { result->autorelease(); return result; }
    delete result; return nullptr;
}
bool Timeline::init(LevelEditorLayer* editor) {
    if (!CCNode::init()) return false;
    m_editor = editor;
    setID("editor-waveform"_spr);
    m_overlay = CCDrawNode::create(); m_overlay->setID("editor-waveform-grid"_spr);
    editor->m_drawGridLayer->addChild(m_overlay, 1);
    scheduleUpdate();
    return true;
}
void Timeline::pollSong() {
    auto path = std::string(CCFileUtils::sharedFileUtils()->fullPathForFilename(m_editor->m_level->getAudioFileName().c_str(), false));
    std::error_code error;
    auto modified = std::filesystem::last_write_time(path, error);
    if (path != m_path || modified != m_modified) {
        m_path = path; m_modified = modified;
        m_source.load(FMODAudioEngine::get()->m_system, path);
    }
    m_songOffset = m_editor->m_levelSettings->m_songOffset;
}
void Timeline::update(float dt) {
    if (m_suspended) return;
    auto config = Config::get();
    if (config.enabled != m_enabled) {
        m_enabled = config.enabled;
        if (!m_enabled) m_source.cancel();
        else {
            m_path.clear(); m_pollTimer = 0;
        }
    }
    setVisible(m_enabled); m_overlay->setVisible(m_enabled);
    if (!m_enabled) return;
    m_pollTimer -= dt;
    if (m_pollTimer <= 0) { pollSong(); m_pollTimer = 1; }
    renderGrid(config);
}
void Timeline::renderGrid(Config const& config) {
    m_overlay->clear();
    if (m_editor->m_levelSettings->m_platformerMode) return;
    auto source = m_source.audio();
    if (!source || !source->sampleRate) return;
    auto grid = m_editor->m_drawGridLayer;
    auto view = CCDirector::sharedDirector()->getWinSize();
    auto pixels = CCDirector::sharedDirector()->getOpenGLView()->getFrameSize().width * CCDirector::sharedDirector()->getContentScaleFactor();
    auto left = grid->convertToNodeSpace({0, 0});
    auto right = grid->convertToNodeSpace({view.width, 0});
    auto columns = std::clamp(static_cast<int>(pixels / config.lineWidth), 1, 4096);
    auto step = (right.x - left.x) / columns;
    if (!std::isfinite(step) || step <= 0) return;
    auto baseline = config.lockToCamera ? grid->convertToNodeSpace({0, config.verticalOffset}).y : m_worldBaseline + config.verticalOffset;
    if (config.lockToCamera) m_worldBaseline = baseline - config.verticalOffset;
    ccColor4F color{31.f / 255, 31.f / 255, 31.f / 255, config.opacity / 255.f};
    auto order = m_editor->getSelectedEditorOrder();
    auto channel = m_editor->getSelectedOrderChannel();
    for (int column = 0; column < columns; ++column) {
        auto x = left.x + column * step;
        auto begin = grid->timeForPos({x, left.y}, order, channel, false, false, false, 0) + m_songOffset;
        auto end = grid->timeForPos({x + step, left.y}, order, channel, false, false, false, 0) + m_songOffset;
        auto peak = source->envelope.atTime(begin, end, source->sampleRate);
        if (peak) {
            auto amplitude = std::min(*peak, 4.f) * config.verticalScale;
            rectangle(m_overlay, x, baseline - amplitude, x + step * .9f, baseline + amplitude, color);
        }
    }
}
void Timeline::onExit() {
    m_suspended = true; m_source.cancel(); m_overlay->setVisible(false);
    CCNode::onExit();
}
void Timeline::onEnter() {
    CCNode::onEnter(); m_suspended = false; m_enabled = false; m_pollTimer = 0;
}
}
