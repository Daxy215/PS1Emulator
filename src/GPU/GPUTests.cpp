#include "GPUTests.h"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Gpu.h"

namespace {
    std::string hex32(uint32_t value) {
        std::ostringstream out;
        out << "0x" << std::hex << std::setw(8) << std::setfill('0') << value;
        return out.str();
    }

    struct Runner {
            int                      passed = 0;
            int                      failed = 0;
            std::vector<std::string> failures;

            void expect(const std::string &name, bool ok, const std::string &detail = {}) {
                if (ok) {
                    passed++;
                    return;
                }

                failed++;
                failures.push_back(detail.empty() ? name : name + ": " + detail);
            }

            void expectEq(const std::string &name, uint64_t got, uint64_t wanted) {
                expect(name, got == wanted, "got " + std::to_string(got) + ", wanted " + std::to_string(wanted));
            }

            void expectStatusBit(const std::string &name, uint32_t status, uint32_t bit, bool wanted) {
                bool got = ((status >> bit) & 1u) != 0;
                expect(name, got == wanted, "status " + hex32(status));
            }

            template<typename Fn>
            void test(const std::string &name, Fn &&fn) {
                try {
                    fn();
                    passed++;
                } catch (const std::exception &e) {
                    failed++;
                    failures.push_back(name + ": threw " + e.what());
                } catch (...) {
                    failed++;
                    failures.push_back(name + ": threw unknown exception");
                }
            }
    };

    Emulator::Gpu makeTimingGpu() { return Emulator::Gpu(false); }

    void testNtscHorizontalTiming(Runner &runner) {
        runner.test("GPU NTSC horizontal timing", [&] {
            Emulator::Gpu gpu = makeTimingGpu();

            runner.expectEq("NTSC ticks per line", gpu.htotal(), 3413);
            runner.expectEq("NTSC total lines", gpu.vtotal(), 263);
            //runner.expectEq("NTSC active start", gpu.hactiveStart(), 488);
            //runner.expectEq("NTSC active end", gpu.hactiveEnd(), 3288);
            runner.expectEq("320px dot divider", gpu.dotClockDivider(), 8);

            runner.expect("initial hblank", gpu.isInHBlank);
            runner.expect("initial vblank", gpu.isInVBlank);
            runner.expectEq("initial dot", gpu.dot, 0);

            //gpu.stepCRTC(gpu.hactiveStart() - 1);
            runner.expectEq("hpos before active", gpu._hpos, 487);
            runner.expect("still in hblank", gpu.isInHBlank);
            runner.expectEq("dot before active", gpu.dot, 60);

            gpu.stepCRTC(1);
            runner.expectEq("hpos active start", gpu._hpos, 488);
            runner.expect("left hblank", !gpu.isInHBlank);
            runner.expectEq("dot at active start", gpu.dot, 61);

            //gpu.stepCRTC(gpu.hactiveEnd() - gpu._hpos);
            runner.expectEq("hpos hblank start", gpu._hpos, 3288);
            runner.expect("entered hblank", gpu.isInHBlank);
            runner.expectEq("dot at hblank start", gpu.dot, 411);

            gpu.stepCRTC(gpu.htotal() - gpu._hpos);
            runner.expectEq("line wrap hpos", gpu._hpos, 0);
            runner.expectEq("line wrap scanline", gpu._scanLine, 1);
            runner.expect("hblank after line wrap", gpu.isInHBlank);
            runner.expectEq("dot after line wrap", gpu.dot, 0);
        });
    }

    void testNtscVerticalTiming(Runner &runner) {
        runner.test("GPU NTSC vertical timing", [&] {
            Emulator::Gpu gpu = makeTimingGpu();

            runner.expect("starts in vblank", gpu.isInVBlank);

            gpu.stepCRTC(gpu.htotal() * gpu.displayLineStart);
            runner.expectEq("active display first line", gpu._scanLine, gpu.displayLineStart);
            runner.expect("left vblank at displayLineStart", !gpu.isInVBlank);

            bool enteredVBlank = gpu.stepCRTC(gpu.htotal() * (gpu.displayLineEnd - gpu.displayLineStart));
            runner.expect("entered vblank result", enteredVBlank);
            runner.expectEq("vblank start line", gpu._scanLine, gpu.displayLineEnd);
            runner.expect("is in vblank", gpu.isInVBlank);

            gpu.stepCRTC(gpu.htotal() * (gpu.vtotal() - gpu._scanLine));
            runner.expectEq("frame wrap line", gpu._scanLine, 0);
            runner.expectEq("frame count", gpu.frames, 1);
            runner.expect("vblank after frame wrap", gpu.isInVBlank);
        });
    }

    void testCpuCycleToCrtcTicks(Runner &runner) {
        runner.test("GPU CPU cycle conversion", [&] {
            Emulator::Gpu gpu = makeTimingGpu();

            bool enteredVBlank = gpu.step(1);
            runner.expect("single CPU cycle did not enter vblank", !enteredVBlank);
            runner.expectEq("first CPU cycle CRTC ticks", gpu.lastGpuCycles, 1);
            runner.expectEq("first CPU cycle hpos", gpu._hpos, 1);
            runner.expectEq("first CPU cycle fraction", gpu._gpuFrac, 264325);

            gpu.step(1);
            runner.expectEq("second CPU cycle CRTC ticks", gpu.lastGpuCycles, 2);
            runner.expectEq("second CPU cycle hpos", gpu._hpos, 3);
            runner.expectEq("second CPU cycle fraction", gpu._gpuFrac, 77066);
        });
    }

    void testPalTiming(Runner &runner) {
        runner.test("GPU PAL timing", [&] {
            Emulator::Gpu gpu = makeTimingGpu();
            gpu.gp1DisplayMode(0x08 | 0x01);

            runner.expectEq("PAL ticks per line", gpu.htotal(), 3406);
            runner.expectEq("PAL total lines", gpu.vtotal(), 314);
            //runner.expectEq("PAL active start", gpu.hactiveStart(), 488);
            //runner.expectEq("PAL active end", gpu.hactiveEnd(), 3300);
            runner.expectEq("PAL 320px dot divider", gpu.dotClockDivider(), 8);

            gpu.stepCRTC(gpu.htotal());
            runner.expectEq("PAL line wrap hpos", gpu._hpos, 0);
            runner.expectEq("PAL line wrap scanline", gpu._scanLine, 1);
        });
    }

    void testStatusOddLineBit(Runner &runner) {
        runner.test("GPU odd line status timing", [&] {
            Emulator::Gpu gpu = makeTimingGpu();

            gpu.stepCRTC(gpu.htotal() * gpu.displayLineStart);
            runner.expectStatusBit("even active line", gpu.status(), 31, false);

            gpu.stepCRTC(gpu.htotal());
            runner.expectStatusBit("odd active line", gpu.status(), 31, true);

            gpu.stepCRTC(gpu.htotal() * (gpu.displayLineEnd - gpu._scanLine));
            runner.expect("back in vblank", gpu.isInVBlank);
            runner.expectStatusBit("odd bit cleared in vblank", gpu.status(), 31, false);
        });
    }
} // namespace

bool GpuTimingTests::runAll() {
    Runner runner;

    testNtscHorizontalTiming(runner);
    testNtscVerticalTiming(runner);
    testCpuCycleToCrtcTicks(runner);
    testPalTiming(runner);
    testStatusOddLineBit(runner);

    std::cerr << "GPU timing tests: " << runner.passed << " passed, " << runner.failed << " failed\n";

    for (const std::string &failure: runner.failures)
        std::cerr << "  " << failure << '\n';

    return runner.failed == 0;
}
