/*
 * sst-jucegui - an open source library of juce widgets
 * built by Surge Synth Team.
 *
 * Copyright 2023-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * sst-jucegui is released under the MIT license, as described
 * by "LICENSE.md" in this repository. This means you may use this
 * in commercial software if you are a JUCE Licensee. If you use JUCE
 * in the open source / GPL3 context, your combined work must be
 * released under GPL3.
 *
 * All source in sst-jucegui available at
 * https://github.com/surge-synthesizer/sst-jucegui
 */

#ifndef INCLUDE_SST_JUCEGUI_UTIL_WHEELCALIBRATION_H
#define INCLUDE_SST_JUCEGUI_UTIL_WHEELCALIBRATION_H

#include <cmath>
#include <juce_gui_basics/juce_gui_basics.h>

/*
 * Shared mouse wheel calibration for windows and linux.
 *
 * Every wheel handler in this library used to bake in its own fudge factor
 * applied to the raw juce delta, which is why calibration drifted apart
 * between widgets and why the same gesture felt different on each platform.
 * This header gives them one common unit: the detent, meaning one physical
 * click of a notched wheel.
 *
 * juce hands us, for a single detent:
 *   windows  0.5 * WHEEL_DELTA / 256 == 60/256   (juce_Windowing_windows.cpp)
 *   x11      50/256                              (juce_XWindowSystem_linux.cpp)
 * and on both of those platforms isSmooth is always false. Precision
 * touchpads on windows report proportionally smaller fractions of a detent,
 * which is why sub-detent motion has to accumulate rather than be discarded.
 *
 * macOS deltas are device dependent and much larger, and the mac paths are
 * deliberately left on their old hand-tuned constants for now, so nothing in
 * here is used from a JUCE_MAC branch.
 */

namespace sst::jucegui::util
{

/*
 * Callers branch on this with "if constexpr" rather than with an #if, so that
 * the paths for the platform you are not on still get compiled and type
 * checked. Wheel behaviour is exactly the kind of thing that only gets built
 * when someone finally boots the other OS, which is how it drifted in the
 * first place.
 */
#if JUCE_MAC
static constexpr bool onMac{true};
#else
static constexpr bool onMac{false};
#endif

#if JUCE_WINDOWS
static constexpr float oneDetent{60.f / 256.f};
#else
static constexpr float oneDetent{50.f / 256.f};
#endif

// How many detents sweep a continuous control across its full range.
static constexpr float detentsPerSweep{64.f};

// Shift is the fine-adjust modifier everywhere, matching the drag behaviour.
static constexpr float fineFactor{0.1f};

/*
 * Global user preference for wheel direction. macOS exposes this as an OS
 * setting and juce reports it via MouseWheelDetails::isReversed, but windows
 * and linux have no equivalent, so the host application supplies it. Off by
 * default, so this is inert until something calls setWheelInverted.
 */
inline bool &wheelInvertedFlag()
{
    static bool f{false};
    return f;
}
inline void setWheelInverted(bool b) { wheelInvertedFlag() = b; }
inline bool isWheelInverted() { return wheelInvertedFlag(); }

/*
 * The signed vertical wheel motion, in detents, with the OS reversal and the
 * user preference both applied. Positive is a wheel-up gesture.
 */
inline float wheelDetents(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
    auto dy = wheel.deltaY;
    if (wheel.isReversed)
        dy = -dy;
    if (isWheelInverted())
        dy = -dy;
    return dy / oneDetent;
}

// As above for horizontal motion. Positive is a tilt-left / swipe-right gesture,
// matching the sign convention juce::Viewport assumes for deltaX.
inline float wheelDetentsX(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
{
    auto dx = wheel.deltaX;
    if (wheel.isReversed)
        dx = -dx;
    if (isWheelInverted())
        dx = -dx;
    return dx / oneDetent;
}

/*
 * Converts a stream of wheel events into whole detent steps for widgets that
 * move in integer increments.
 *
 * Two things this gets right that a plain running total does not. Crossing the
 * threshold subtracts a detent rather than zeroing, so slow sub-detent motion
 * keeps adding up instead of losing its remainder. And a change of direction
 * discards the residual from the previous direction, so the first click back
 * the other way acts immediately rather than being swallowed working the old
 * residual off.
 */
struct DetentAccumulator
{
    double accum{0};

    void reset() { accum = 0; }

    // Returns the number of whole detents this event completes, usually 0 or +/-1.
    int operator()(const juce::MouseEvent &e, const juce::MouseWheelDetails &wheel)
    {
        auto d = wheelDetents(e, wheel);
        if (std::fabs(d) < 1e-6)
            return 0;

        if ((d < 0) != (accum < 0))
            accum = 0;

        accum += d;

        // nudge so a detent that lands a float epsilon short still counts
        auto nudged = accum + (accum > 0 ? 1e-4 : -1e-4);
        auto steps = (int)std::trunc(nudged);
        accum -= steps;
        return steps;
    }
};

} // namespace sst::jucegui::util
#endif // INCLUDE_SST_JUCEGUI_UTIL_WHEELCALIBRATION_H
