#pragma once
#include <vector>
#include "adsr.h"
#include "regs.h"


union Reg16 {
    uint16_t _reg;
    uint8_t _byte[2];

    Reg16() : _reg(0) {}

    void write(int n, uint8_t v) {
        if (n >= 2) return;
        _byte[n] = v;
    }

    uint8_t read(int n) const {
        if (n >= 2) return 0;
        return _byte[n];
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(_reg);
    }
};

union Reg32 {
    uint32_t _reg;
    uint8_t _byte[4];

    Reg32() : _reg(0) {}

    void write(int n, uint8_t v) {
        if (n >= 4) return;
        _byte[n] = v;
    }

    uint8_t read(int n) const {
        if (n >= 4) return 0;
        return _byte[n];
    }

    void setBit(int n, bool v) {
        if (n >= 32) return;
        _reg &= ~(1 << n);
        _reg |= (v << n);
    }

    bool getBit(int n) {
        if (n >= 32) return false;
        return (_reg & (1 << n)) != 0;
    }

    template <class Archive>
    void serialize(Archive& ar) {
        ar(_reg);
    }
};

namespace spu {
struct Voice {
    enum class State { Attack, Decay, Sustain, Release, Off };
    enum class Mode { ADSR, Noise };
    union Counter {
        struct {
            uint32_t : 4;
            uint32_t index : 8;  // Interpolation index
            uint32_t sample : 5;
            uint32_t : 15;
        };
        uint32_t _reg = 0;
    };
    SweepVolume volume;
    Reg16 sampleRate;
    Reg16 startAddress;
    ADSR adsr;
    Reg16 adsrVolume;
    Reg16 repeatAddress;
    bool ignoreLoadRepeatAddress;

    Reg16 currentAddress;
    Counter counter;
    State state;
    Mode mode;
    bool pitchModulation;
    bool reverb;

    int adsrWaitCycles;
    bool loopEnd;
    bool loadRepeatAddress;
    bool flagsParsed;

    int16_t sample;  // Used for Pitch Modulation

    bool enabled;     // Allows for muting individual channels
    uint64_t cycles;  // For dismissing KeyOff write right after KeyOn

    // ADPCM decoding
    int32_t prevSample[2];
    std::vector<int16_t> decodedSamples;
    std::vector<int16_t> prevDecodedSamples;

    Voice();
    Envelope getCurrentPhase();
    State nextState(State current);
    void processEnvelope();
    void parseFlags(uint8_t flags);

    void keyOn(uint64_t cycles = 0);
    void keyOff(uint64_t cycles = 0);

    template <class Archive>
    void serialize(Archive& ar) {
        ar(volume._reg, sampleRate, startAddress);
        ar(adsr._reg, adsrVolume);
        ar(repeatAddress);
        ar(ignoreLoadRepeatAddress);
        ar(currentAddress);
        ar(counter._reg);
        ar(state);
        ar(mode);
        ar(pitchModulation);
        ar(reverb);
        ar(adsrWaitCycles);
        ar(loopEnd);
        ar(loadRepeatAddress);
        ar(flagsParsed);
        ar(sample);
        ar(cycles);
        ar(prevSample);
        ar(decodedSamples);
        ar(prevDecodedSamples);
    }
};
}  // namespace spu
