#include "waveform.hpp"
#include <Geode/fmod/fmod.hpp>
#include <Geode/fmod/fmod_errors.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

static void check(bool condition, char const* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
static void ok(FMOD_RESULT result) {
    if (result != FMOD_OK) { std::cerr << FMOD_ErrorString(result) << '\n'; std::exit(1); }
}
static void writeLE(std::ofstream& file, unsigned value, unsigned bytes) {
    for (unsigned i = 0; i < bytes; ++i) file.put(static_cast<char>(value >> (i * 8)));
}
static void wav(std::filesystem::path const& path, unsigned rate, bool stereo, unsigned bits = 16, bool silent = false) {
    std::ofstream file(path, std::ios::binary);
    unsigned channels = stereo ? 2 : 1, sampleBytes = bits / 8, dataBytes = rate * channels * sampleBytes;
    file.write("RIFF", 4); writeLE(file, 36 + dataBytes, 4); file.write("WAVEfmt ", 8);
    writeLE(file, 16, 4); writeLE(file, 1, 2); writeLE(file, channels, 2);
    writeLE(file, rate, 4); writeLE(file, rate * channels * sampleBytes, 4);
    writeLE(file, channels * sampleBytes, 2); writeLE(file, bits, 2);
    file.write("data", 4); writeLE(file, dataBytes, 4);
    for (unsigned frame = 0; frame < rate; ++frame) {
        int value = silent ? 0 : frame % 128 < 64 ? (1 << (bits - 2)) : -(1 << (bits - 2));
        writeLE(file, bits == 8 ? value + 128 : unsigned(value), sampleBytes);
        if (stereo) writeLE(file, bits == 8 ? -value + 128 : unsigned(-value), sampleBytes);
    }
}
int main() {
    using namespace waveform;
    FMOD::System* system = nullptr;
    ok(FMOD::System_Create(&system));
    ok(system->setOutput(FMOD_OUTPUTTYPE_NOSOUND_NRT));
    ok(system->setSoftwareFormat(48000, FMOD_SPEAKERMODE_STEREO, 0));
    ok(system->setDSPBufferSize(256, 2));
    ok(system->init(32, FMOD_INIT_NORMAL, nullptr));
    auto root = std::filesystem::temp_directory_path() / ("editor-waveform-fmod-tests-" + std::to_string(getpid()));
    std::filesystem::create_directories(root);
    auto mono = root / "mono.wav", stereo = root / "stereo.wav";
    wav(mono, 24000, false); wav(stereo, 48000, true);
    std::atomic<bool> cancelled{false};
    auto audio = decodeAudio(system, mono.string(), cancelled);
    check(audio.error.empty() && audio.sampleRate == 24000 && audio.frames == 24000, "FMOD metadata and chunked decode");
    check(audio.envelope.peak(0, audio.envelope.size()) == .5f, "mono normalization");
    audio = decodeAudio(system, stereo.string(), cancelled);
    check(audio.error.empty() && audio.envelope.peak(0, audio.envelope.size()) == .5f, "opposite-phase stereo peaks survive");
    for (unsigned bits : {8u, 24u, 32u}) {
        auto path = root / ("pcm" + std::to_string(bits) + ".wav");
        wav(path, 24000, false, bits, true);
        audio = decodeAudio(system, path.string(), cancelled);
        check(audio.error.empty() && audio.envelope.peak(0, audio.envelope.size()) == 0, "FMOD PCM silence across bit depths");
        wav(path, 24000, false, bits);
        audio = decodeAudio(system, path.string(), cancelled);
        check(audio.error.empty() && audio.envelope.peak(0, audio.envelope.size()) == .5f, "FMOD PCM amplitude across bit depths");
        std::filesystem::remove(path);
    }
    check(!decodeAudio(system, (root / "missing.wav").string(), cancelled).error.empty(), "missing file handled");
    cancelled = true;
    check(decodeAudio(system, mono.string(), cancelled).frames == 0, "decode cancellation");
    ok(system->close()); ok(system->release());
    std::filesystem::remove(mono); std::filesystem::remove(stereo); std::filesystem::remove(root);
    std::cout << "Local FMOD decode tests passed\n";
}
