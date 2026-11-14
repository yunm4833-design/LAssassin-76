// SPDX-License-Identifier: AGPL-3.0-or-later
// LDR heat-target and charge/cool architecture adapted from ZeroComp:
// Copyright (C) 2026 Jun Murakami
// https://github.com/Jun-Murakami/ZeroComp
// Changes: double/expm1 integration, fixed T4 time constants, equal fast/slow
// weighting (not max), interpolate physical seconds rather than coefficients.
#pragma once
#include <algorithm>
#include <cmath>

namespace assassin
{
class OpticalMemory
{
  public:
    struct State
    {
        double fastDb, slowDb, heat, slowSeconds;
    };
    void prepare(double sampleRate) noexcept
    {
        fs = std::isfinite(sampleRate) && sampleRate > 0 ? sampleRate : 48000.;
        attack = step(.010);
        fastRelease = step(.060 / std::log(10.)); // 90% of fast half released at 60 ms
        heatUp = step(1.0);
        coolDown = step(3.0);
        reset();
    }
    void reset() noexcept { fast = slow = heat = 0; }
    float process(float targetDb) noexcept
    {
        const double target = std::isfinite(targetDb) ? std::clamp(double(targetDb), 0., 50.) : 0.;
        fast += (target > fast ? attack : fastRelease) * (target - fast);
        // ZeroComp's memory is driven by realized fast-envelope GR, not the
        // instantaneous demanded GR: brief uncompressed spikes cannot preheat it.
        const double heatTarget = std::clamp(fast / 18., 0., 1.);
        heat += (heatTarget > heat ? heatUp : coolDown) * (heatTarget - heat);
        const double slowRelease = step(1. + 14. * heat);
        slow += (target > slow ? attack : slowRelease) * (target - slow);
        return float(.5 * (fast + slow));
    }
    State state() const noexcept { return {fast, slow, heat, 1. + 14. * heat}; }

  private:
    double step(double seconds) const noexcept { return -std::expm1(-1. / (seconds * fs)); }
    double fs = 48000, fast = 0, slow = 0, heat = 0;
    double attack = 0, fastRelease = 0, heatUp = 0, coolDown = 0;
};
} // namespace assassin
