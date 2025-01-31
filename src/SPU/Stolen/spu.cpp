#include "spu.h"
#include <array>
#include <functional>
#include <vector>
#include "interpolation.h"
#include "reverb.h"
#include "sample.h"
#include "../../Memory/IRQ.h"

using namespace spu;

SPU::SPU() {
    verbose = 0;//config.debug.log.spu;
    ram.fill(0);
    audioBufferPos = 0;
    captureBufferIndex = 0;
}

void SPU::step(device::cdrom::CDROM* cdrom) {
    Sample sumLeft = 0, sumReverbLeft = 0;
    Sample sumRight = 0, sumReverbRight = 0;

    noise.doNoise(control.noiseFrequencyStep, control.noiseFrequencyShift);

    for (int v = 0; v < VOICE_COUNT; v++) {
        Voice& voice = voices[v];

        if (voice.state == Voice::State::Off) continue;

        if (voice.decodedSamples.empty()) {
            auto block = readBlock(voice.currentAddress._reg * 8);
            //voice.decodedSamples = ADPCM::decode(block.data(), voice.prevSample);
            voice.flagsParsed = false;
        }

        voice.processEnvelope();

        uint32_t step = voice.sampleRate._reg;
        if (voice.pitchModulation && v > 0) {
            int32_t factor = voices[v - 1].sample + 0x8000;
            step = (step * factor) >> 15;
            step &= 0xffff;
        }
        if (step > 0x3fff) step = 0x4000;

        Sample sample;
        if (voice.mode == Voice::Mode::Noise) {
            sample = noise.getNoiseLevel();
        } else {
            //sample = interpolate(voice, voice.counter.sample, voice.counter.index);
        }
        sample *= voice.adsrVolume._reg;
        voice.sample = sample;

        if (voice.enabled) {
            sumLeft += sample * voice.volume.getLeft();
            sumRight += sample * voice.volume.getRight();

            if (voice.reverb) {
                sumReverbLeft += sample * voice.volume.getLeft();
                sumReverbRight += sample * voice.volume.getRight();
            }
        }

        voice.counter._reg += step;
        if (voice.counter.sample >= 28) {
            // Overflow, parse next ADPCM block
            voice.counter.sample -= 28;
            voice.currentAddress._reg += 2;
            voice.prevDecodedSamples = voice.decodedSamples;
            voice.decodedSamples.clear();

            if (voice.loadRepeatAddress) {
                voice.loadRepeatAddress = false;
                voice.currentAddress = voice.repeatAddress;
            }
        }

        if (!voice.flagsParsed) {
            voice.parseFlags(ram[voice.currentAddress._reg * 8 + 1]);
        }
    }

    if (!control.unmute) {
        sumLeft = 0;
        sumRight = 0;
    }
    // TODO: Check if SPU mute affect sumReverb for voices

    // Mix with cd
    Sample cdLeft = 0, cdRight = 0;
    /*if (!cdrom->audio.empty()) {
        std::tie(cdLeft, cdRight) = cdrom->audio.front();

        // TODO: Refactor to use ring buffer
        cdrom->audio.pop_front();

        if (control.cdEnable) {
            sumLeft += cdLeft * cdVolume.getLeft();
            sumRight += cdRight * cdVolume.getRight();

            if (control.cdReverb) {
                sumReverbLeft += cdLeft * cdVolume.getLeft();
                sumReverbRight += cdRight * cdVolume.getRight();
            }
        }
    }
    */

    if (reverbCounter++ % 2 == 0) {
        std::tie(reverbLeft, reverbRight) = doReverb(this, std::make_tuple(sumReverbLeft, sumReverbRight));
    }
    sumLeft += reverbLeft;
    sumRight += reverbRight;

    sumLeft *= std::min<int16_t>(0x3fff, mainVolume.getLeft()) * 2;
    sumRight *= std::min<int16_t>(0x3fff, mainVolume.getRight()) * 2;

    audioBuffer[audioBufferPos] = sumLeft;
    audioBuffer[audioBufferPos + 1] = sumRight;

    audioBufferPos += 2;
    if (audioBufferPos >= AUDIO_BUFFER_SIZE) {
        if (recording) {
            //std::copy(audioBuffer.begin(), audioBuffer.end(), std::back_inserter(recordBuffer));
        }
        audioBufferPos = 0;
        bufferReady = true;
    }

    const uint32_t cdLeftAddress = 0x000 + captureBufferIndex;
    const uint32_t cdRightAddress = 0x400 + captureBufferIndex;
    const uint32_t voice1Address = 0x800 + captureBufferIndex;
    const uint32_t voice3Address = 0xC00 + captureBufferIndex;

    captureBufferIndex = (captureBufferIndex + 2) & 0x3ff;

    memoryWrite16(cdLeftAddress, cdLeft);
    memoryWrite16(cdRightAddress, cdRight);
    memoryWrite16(voice1Address, voices[1].sample);
    memoryWrite16(voice3Address, voices[3].sample);
}

uint8_t SPU::readVoice(uint32_t address) const {
    int voice = address / 0x10;
    int reg = address % 0x10;

    switch (reg) {
        case 0:
        case 1:
        case 2:
        case 3: return voices[voice].volume.read(reg);

        case 4:
        case 5: return voices[voice].sampleRate.read(reg - 4);

        case 6:
        case 7: return voices[voice].startAddress.read(reg - 6);

        case 8:
        case 9:
        case 10:
        case 11: return voices[voice].adsr.read(reg - 8);

        case 12:
        case 13: return voices[voice].adsrVolume.read(reg - 12);

        case 14:
        case 15: return voices[voice].repeatAddress.read(reg - 14);

        default: return 0;
    }
}

void SPU::writeVoice(uint32_t address, uint8_t data) {
    int voice = address / 0x10;
    int reg = address % 0x10;
    switch (reg) {

        case 0:
        case 1:
        case 2:
        case 3:
            voices[voice].volume.write(reg, data);
            return;

        case 4:
        case 5: voices[voice].sampleRate.write(reg - 4, data); return;

        case 6:
        case 7:
            voices[voice].counter._reg = 0;
            voices[voice].prevDecodedSamples.clear();  // TODO: Not sure is this is what real hardware does
            voices[voice].startAddress.write(reg - 6, data);
            return;

        case 8:
        case 9:
        case 10:
        case 11: voices[voice].adsr.write(reg - 8, data); return;

        case 12:
        case 13: voices[voice].adsrVolume.write(reg - 12, data); return;

        case 14:
        case 15:
            voices[voice].repeatAddress.write(reg - 14, data);
            voices[voice].ignoreLoadRepeatAddress = true;
            return;

        default: return;
    }
}

uint8_t SPU::read(uint32_t address) {
// Helper to extract given flag from all voices
#define READ_FOR_EACH_VOICE(BYTE, FIELD)                                        \
    [&]() {                                                                     \
        uint8_t data = 0;                                                       \
        for (unsigned v = (BYTE)*8; v < (BYTE)*8 + 8 && v < VOICE_COUNT; v++) { \
            data |= static_cast<uint8_t>(voices[v].FIELD) << (v % 8);           \
        }                                                                       \
        return data;                                                            \
    }()

    address += BASE_ADDRESS;

    if (address >= 0x1f801c00 && address < 0x1f801c00 + 0x10 * VOICE_COUNT) {
        return readVoice(address - 0x1f801c00);
    }

    if (address >= 0x1f801d80 && address <= 0x1f801d83) {  // Main Volume L/R
        return mainVolume.read(address - 0x1f801d80);
    }

    if (address >= 0x1f801d84 && address <= 0x1f801d87) {  // Reverb Volume L/R
        return reverbVolume.read(address - 0x1f801d84);
    }

    if (address >= 0x1f801da4 && address <= 0x1f801da5) {  // IRQ (used by Vib-Ribbon)
        return irqAddress.read(address - 0x1f801da4);
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        return dataAddress.read(address - 0x1f801da6);
    }

    if (address >= 0x1f801daa && address <= 0x1f801dab) {  // SPUCNT
        return control._byte[address - 0x1f801daa];
    }

    if (address >= 0x1f801db0 && address <= 0x1f801db3) {  // CD Volume L/R
        return cdVolume.read(address - 0x1f801db0);
    }

    if (address >= 0x1F801DB4 && address <= 0x1F801DB7) {  // External input Volume L/R
        return extVolume.read(address - 0x1F801DB4);
    }

    if (address >= 0x1F801D88 && address <= 0x1F801D8b) {  // Voice Key On
        // Shouldn't be read, but some games do, just return last value
        return _keyOn.read(address - 0x1F801D88);
    }

    if (address >= 0x1F801D8c && address <= 0x1F801D8f) {  // Voice Key Off
        // Shouldn't be read, but some games do
        return _keyOff.read(address - 0x1F801D8c);
    }

    if (address >= 0x1F801D90 && address <= 0x1F801D93) {  // Pitch modulation enable flags
        return READ_FOR_EACH_VOICE(address - 0x1F801D90, pitchModulation);
    }

    if (address >= 0x1f801d9c && address <= 0x1f801d9f) {  // Voice ON/OFF status (ENDX)
        return READ_FOR_EACH_VOICE(address - 0x1f801d9c, loopEnd);
    }

    if (address >= 0x1F801D94 && address <= 0x1F801D97) {  // Voice noise mode
        return READ_FOR_EACH_VOICE(address - 0x1F801D94, mode);
    }

    if (address >= 0x1f801d98 && address <= 0x1f801d9b) {  // Voice Reverb
        return READ_FOR_EACH_VOICE(address - 0x1f801d98, reverb);
    }

    if (address >= 0x1F801DA2 && address <= 0x1F801DA3) {  // Reverb Work area start
        // TODO: Breaks Doom if returning correct value, why ?
        return reverbBase.read(address - 0x1F801DA2);
    }

    if (address >= 0x1f801da8 && address <= 0x1f801da9) {  // SPU RAM read
        if (currentDataAddress >= RAM_SIZE) {
            currentDataAddress %= RAM_SIZE;
        }

        return memoryRead8(currentDataAddress++);
    }

    if (address >= 0x1f801dac && address <= 0x1f801dad) {  // Data Transfer Control
        return dataTransferControl._byte[address - 0x1f801dac];
    }

    if (address >= 0x1f801dae && address <= 0x1f801daf) {  // SPUSTAT
        return status._byte[address - 0x1f801dae];
    }

    if (address >= 0x1f801db8 && address <= 0x1f801dbb) {  // Current Main Volume L/R ?? (used by MGS)
        return mainVolume.read(address - 0x1f801db8);
    }

    if (address >= 0x1f801e00 && address < 0x1f801e00 + 4 * VOICE_COUNT) {  // Internal Voice volume (used by MGS)
        // TODO: Not sure if it is channel volume or processed sample volume
        int voice = (address - 0x1f801e00) / 4;
        int byte = (address - 0x1f801e00) % 4;
        return voices[voice].volume.read(byte);
    }
    
    return 0;
#undef READ_FOR_EACH_VOICE
}

void SPU::write(uint32_t address, uint8_t data) {
    // Helper to set given flag for all voices
    auto FOR_EACH_VOICE = [&](unsigned BYTE, std::function<void(int, bool)> FUNC) {
        for (unsigned v = BYTE * 8; v < BYTE * 8 + 8 && v < VOICE_COUNT; v++) {
            FUNC(v, data & (1 << (v % 8)));
        }
    };

    address += BASE_ADDRESS;

    if (address >= 0x1f801c00 && address < 0x1f801c00 + 0x10 * VOICE_COUNT) {
        writeVoice(address - 0x1f801c00, data);
        return;
    }
    if (verbose) {
        if (address >= 0x1f801da8 && address <= 0x1f801da9) {
        } else if (address >= 0x1f801d88 && address <= 0x1f801d8b) {
        } else if (address >= 0x1f801d8c && address <= 0x1f801d8f) {
        } else if (address >= 0x1f801d98 && address <= 0x1f801d9b) {
        }
    }

    if (address >= 0x1f801d80 && address <= 0x1f801d83) {  // Main Volume L/R
        mainVolume.write(address - 0x1f801d80, data);
        return;
    }

    if (address >= 0x1f801d84 && address <= 0x1f801d87) {  // Reverb Volume L/R
        reverbVolume.write(address - 0x1f801d84, data);
        return;
    }

    if (address >= 0x1f801d88 && address <= 0x1f801d8b) {  // Voices Key On
        FOR_EACH_VOICE(address - 0x1f801d88, [&](int v, bool bit) {
            if (control.spuEnable && bit) voices[v].keyOn(100);
        });
        return;
    }

    if (address >= 0x1f801d8c && address <= 0x1f801d8f) {  // Voices Key Off
        FOR_EACH_VOICE(address - 0x1f801d8c, [&](int v, bool bit) {
            if (control.spuEnable && bit) voices[v].keyOff(100);
        });
        return;
    }

    if (address >= 0x1F801D90 && address <= 0x1F801D93) {  // Pitch modulation enable flags
        FOR_EACH_VOICE(address - 0x1F801D90, [&](int v, bool bit) {
            if (v > 0) voices[v].pitchModulation = bit;
        });
        return;
    }

    if (address >= 0x1F801D94 && address <= 0x1F801D97) {  // Voice noise mode
        FOR_EACH_VOICE(address - 0x1F801D94, [&](int v, bool bit) { voices[v].mode = bit ? Voice::Mode::Noise : Voice::Mode::ADSR; });
        return;
    }

    if (address >= 0x1f801d98 && address <= 0x1f801d9b) {  // Voice Reverb
        FOR_EACH_VOICE(address - 0x1f801d98, [&](int v, bool bit) {
            voices[v].reverb = bit;
        });
        return;
    }

    if (address >= 0x1f801d9c && address <= 0x1f801d9f) {  // Voice ON/OFF status (ENDX)
        // Writes to this address should be ignored
        return;
    }

    if (address >= 0x1F801DA2 && address <= 0x1F801DA3) {  // Reverb Work area start
        reverbBase.write(address - 0x1F801DA2, data);
        if (address == 0x1F801DA3) {
            reverbCurrentAddress = reverbBase._reg * 8;
        }
        return;
    }

    if (address >= 0x1f801da4 && address <= 0x1f801da5) {  // IRQ Address
        irqAddress.write(address - 0x1f801da4, data);
        return;
    }

    if (address >= 0x1f801da6 && address <= 0x1f801da7) {  // Data address
        dataAddress.write(address - 0x1f801da6, data);
        currentDataAddress = dataAddress._reg * 8;
        return;
    }

    if (address >= 0x1f801da8 && address <= 0x1f801da9) {  // Data FIFO
        if (currentDataAddress >= RAM_SIZE) {
            currentDataAddress %= RAM_SIZE;
        }

        memoryWrite8(currentDataAddress++, data);
        return;
    }

    if (address >= 0x1f801daa && address <= 0x1f801dab) {  // SPUCNT
        control._byte[address - 0x1f801daa] = data;

        status.currentMode = control._reg & 0x3f;
        if (!control.irqEnable) {
            status.irqFlag = false;
        }
        if (!control.spuEnable) {
            for (auto& v : voices) {
                v.adsrVolume._reg = 0;
            }
        }
        return;
    }

    if (address >= 0x1f801dac && address <= 0x1f801dad) {  // Data Transfer Control
        dataTransferControl._byte[address - 0x1f801dac] = data;
        return;
    }

    if (address >= 0x1f801db0 && address <= 0x1f801db3) {  // CD Volume L/R
        cdVolume.write(address - 0x1f801db0, data);
        return;
    }

    if (address >= 0x1F801DB4 && address <= 0x1F801DB7) {  // External input Volume L/R
        extVolume.write(address - 0x1F801DB4, data);
        return;
    }

    if (address >= 0x1F801DC0 && address <= 0x1F801DFF) {  // Reverb registers
        auto reg = (address - 0x1F801DC0) / 2;
        auto byte = (address - 0x1F801DC0) % 2;
        reverbRegisters[reg].write(byte, data);
        return;
    }
}

uint8_t SPU::memoryRead8(uint32_t address) {
    if (control.irqEnable && address == irqAddress._reg * 8) {
        status.irqFlag = true;
        IRQ::trigger(IRQ::SPU);
    }

    return ram[address];
}

void SPU::memoryWrite8(uint32_t address, uint8_t data) {
    ram[address] = data;

    if (control.irqEnable && address == irqAddress._reg * 8) {
        status.irqFlag = true;
        IRQ::trigger(IRQ::SPU);
    }
}

void SPU::memoryWrite16(uint32_t address, uint16_t data) {
    memoryWrite8(address, (uint8_t)data);
    memoryWrite8(address + 1, (uint8_t)(data >> 8));
}

std::array<uint8_t, 16> SPU::readBlock(uint32_t address) {
    if (control.irqEnable && address == irqAddress._reg * 8) {
        status.irqFlag = true;
        IRQ::trigger(IRQ::SPU);
    }

    std::array<uint8_t, 16> buf;
    std::copy(ram.begin() + address, ram.begin() + address + 16, buf.begin());
    return buf;
}

void SPU::dumpRam() {
    std::vector<uint8_t> ram;
    ram.assign(this->ram.begin(), this->ram.end());
    //putFileContents("spu.bin", ram);
}
