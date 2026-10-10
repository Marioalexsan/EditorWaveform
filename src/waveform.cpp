#include "waveform.hpp"
#ifndef WAVEFORM_AUDIO_ONLY
#include <Geode/utils/async.hpp>
#include <Geode/loader/Log.hpp>
#endif
#include <Geode/fmod/fmod.hpp>
#include <Geode/fmod/fmod_errors.h>

namespace waveform {
SourceAudio decodeAudio(FMOD::System* system, std::string const& path, std::atomic<bool> const& cancelled) {
    SourceAudio result;
    FMOD::Sound* raw = nullptr;
    auto status = system->createSound(path.c_str(), FMOD_OPENONLY | FMOD_CREATESTREAM, nullptr, &raw);
    if (status != FMOD_OK) { result.error = FMOD_ErrorString(status); return result; }
    auto release = [](FMOD::Sound* sound) { sound->release(); };
    std::unique_ptr<FMOD::Sound, decltype(release)> sound(raw, release);
    FMOD_SOUND_FORMAT format;
    int channels = 0;
    float rate = 0;
    if (sound->getFormat(nullptr, &format, &channels, nullptr) != FMOD_OK ||
        sound->getDefaults(&rate, nullptr) != FMOD_OK || !std::isfinite(rate) || rate <= 0 || channels <= 0 || channels > 32) {
        result.error = "Invalid audio metadata"; return result;
    }
    PCMFormat pcm;
    switch (format) {
        case FMOD_SOUND_FORMAT_PCM8: pcm = PCMFormat::Signed8; break;
        case FMOD_SOUND_FORMAT_PCM16: pcm = PCMFormat::Signed16; break;
        case FMOD_SOUND_FORMAT_PCM24: pcm = PCMFormat::Signed24; break;
        case FMOD_SOUND_FORMAT_PCM32: pcm = PCMFormat::Signed32; break;
        case FMOD_SOUND_FORMAT_PCMFLOAT: pcm = PCMFormat::Float32; break;
        default: result.error = "Unsupported decoded audio format"; return result;
    }
    result.sampleRate = rate;
    auto sampleBytes = bytesPerSample(pcm);
    auto frameBytes = sampleBytes * channels;
    std::array<uint8_t, 65536> buffer{};
    auto request = buffer.size() / frameBytes * frameBytes;
    float peak = 0;
    unsigned binFrames = 0;
    while (!cancelled.load(std::memory_order_relaxed)) {
        unsigned bytes = 0;
        status = sound->readData(buffer.data(), static_cast<unsigned>(request), &bytes);
        if (status != FMOD_OK && status != FMOD_ERR_FILE_EOF) {
            result.error = FMOD_ErrorString(status); break;
        }
        if (bytes > request || bytes % frameBytes) { result.error = "Truncated PCM frame"; break; }
        for (size_t offset = 0; offset < bytes; offset += frameBytes) {
            for (int channel = 0; channel < channels; ++channel) {
                peak = std::max(peak, std::abs(pcmSample(std::span(buffer).subspan(offset + channel * sampleBytes, sampleBytes), pcm)));
            }
            ++result.frames;
            if (++binFrames == BinFrames) {
                if (!result.envelope.append(peak)) { result.error = "Main-song preview reached its memory limit"; return result; }
                binFrames = 0; peak = 0;
            }
        }
        if (status == FMOD_ERR_FILE_EOF || !bytes) break;
    }
    if (binFrames && !result.envelope.append(peak)) result.error = "Main-song preview reached its memory limit";
    if (!result.frames && result.error.empty()) result.error = "Audio file is empty";
    return result;
}

#ifndef WAVEFORM_AUDIO_ONLY
struct SourceLoader::Impl {
    geode::async::TaskHolder<SourceAudio> task;
    std::shared_ptr<std::atomic<bool>> cancelled;
    std::shared_ptr<SourceAudio const> audio;
};
SourceLoader::SourceLoader() : m_impl(std::make_unique<Impl>()) {}
SourceLoader::~SourceLoader() { cancel(); }
void SourceLoader::cancel() {
    if (m_impl->cancelled) m_impl->cancelled->store(true, std::memory_order_relaxed);
    m_impl->task.cancel();
}
void SourceLoader::load(FMOD::System* system, std::string path) {
    cancel();
    m_impl->audio.reset();
    auto cancelled = m_impl->cancelled = std::make_shared<std::atomic<bool>>(false);
    m_impl->task.spawn(geode::async::runtime().spawnBlocking<SourceAudio>([system, path = std::move(path), cancelled] {
        try { return decodeAudio(system, path, *cancelled); }
        catch (std::exception const& error) { SourceAudio result; result.error = error.what(); return result; }
    }), [this](SourceAudio result) {
        if (!result.error.empty()) geode::log::warn("Main-song waveform: {}", result.error);
        m_impl->audio = std::make_shared<SourceAudio>(std::move(result));
    });
}
std::shared_ptr<SourceAudio const> SourceLoader::audio() const { return m_impl->audio; }
#endif

}
