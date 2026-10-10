#include "waveform.hpp"
#include <cstdlib>
#include <iostream>

static void check(bool condition, char const* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
int main() {
    using namespace waveform;
    auto near = [](float a, float b) { return std::abs(a - b) < 1e-6f; };
    std::array<uint8_t, 4> zero{};
    check(pcmSample(zero, PCMFormat::Signed16) == 0, "PCM silence");
    for (auto format : {PCMFormat::Signed8, PCMFormat::Signed16, PCMFormat::Signed24, PCMFormat::Signed32}) {
        auto bytes = bytesPerSample(format);
        auto sample = zero;
        sample[bytes - 1] = 0x80;
        check(pcmSample(sample, format) == -1, "negative PCM extremum");
        sample.fill(0xff);
        sample[bytes - 1] = 0x7f;
        check(near(pcmSample(sample, format), float(1.0 - std::ldexp(1.0, -(int(bytes * 8) - 1)))), "positive PCM extremum");
    }
    auto bits = std::bit_cast<uint32_t>(-.5f);
    std::array<uint8_t, 4> floating{};
    for (unsigned i = 0; i < 4; ++i) floating[i] = (bits >> (i * 8)) & 255;
    check(pcmSample(floating, PCMFormat::Float32) == -.5f, "float PCM");
    check(pcmSample(std::span(zero).first(1), PCMFormat::Signed24) == 0, "truncated sample");
    Envelope envelope;
    std::vector<float> expected;
    for (int i = 0; i < 4097; ++i) {
        auto value = float((i * 7919) % 997) / 997;
        check(envelope.append(value), "envelope append"); expected.push_back(value);
        for (size_t begin : {size_t(0), expected.size() / 3, expected.size() - 1}) {
            check(envelope.peak(begin, expected.size()) == *std::max_element(expected.begin() + begin, expected.end()), "peak pyramid odd tail/range");
        }
    }
    check(!envelope.atTime(-10, -1, 48000), "negative times do not wrap to huge indices");
    check(envelope.atTime(-1, 64.0 / 48000, 48000) == expected[0], "range crossing zero");
    check(!envelope.atTime(NAN, 1, 48000), "nonfinite time");
    Envelope bounded(100);
    check(!bounded.append(1), "memory budget enforced");
    std::cout << "PCM, peak ranges and memory budgets passed\n";
}
