#pragma once
#include <algorithm>
#include <array>
namespace assassin
{
// Derived from SignalTrain LA2A Dataset 1.1, Colburn/Hawley (CC BY 4.0).
// https://doi.org/10.5281/zenodo.3824876 ; provenance: Documentation/Calibration.
// Measured SignalTrain operating points, not a full-range hardware pot fit.
// Endpoints are engineering anchors; interpolation between measured points is
// deliberately monotone and piecewise linear, with no polynomial overshoot.
struct OpticalCalibration
{
    struct Point
    {
        float panel, effective, trim;
    };
    struct Result
    {
        float peak, trimDb;
    };
    template <size_t N> static Result interpolate(float peak, const std::array<Point, N> &points)
    {
        peak = std::clamp(peak, 0.f, 100.f);
        for (size_t i = 1; i < N; ++i)
            if (peak <= points[i].panel)
            {
                const auto &a = points[i - 1], &b = points[i];
                const float t = (peak - a.panel) / (b.panel - a.panel);
                return {a.effective + t * (b.effective - a.effective), a.trim + t * (b.trim - a.trim)};
            }
        return {points.back().effective, points.back().trim};
    }
    static Result map(float peak, float limit, float enabled)
    {
        constexpr std::array<Point, 3> compress{
            {{0, 0, 0}, {65, 34.4746196f, 2.2417604f}, {100, 100, 2.2417604f}}};
        constexpr std::array<Point, 4> limiting{{{0, 0, 0},
                                                 {65, 20.9498472f, -.0795498f},
                                                 {80, 53.7154437f, 10.6020103f},
                                                 {100, 100, 10.6020103f}}};
        const auto c = interpolate(peak, compress), l = interpolate(peak, limiting);
        const float mode = std::clamp(limit, 0.f, 1.f), blend = std::clamp(enabled, 0.f, 1.f);
        return {peak + blend * (c.peak + mode * (l.peak - c.peak) - peak),
                blend * (c.trimDb + mode * (l.trimDb - c.trimDb))};
    }
};
} // namespace assassin
