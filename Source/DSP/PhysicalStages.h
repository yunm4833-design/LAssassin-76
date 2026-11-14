#pragma once
// Adapted circuit sections from Paulllux/fetcomp-dsp (MIT).
// Copyright (c) 2026 Paul Ulrix. License: ThirdParty/fetcomp-dsp/LICENSE.
// LAssassin adaptations: standard-library math, double states, stable quadratic
// root, passive gain limits, zero hidden makeup, and bounded incremental slope.
#include <algorithm>
#include <cmath>

namespace assassin
{
struct JfetCell
{
    static constexpr double seriesResistance = 27000.0, beta = .010 / 9.0, voltsPerUnit = .03;
    // Desired attenuation determines the small-signal shunt conductance.
    static double conductance(float reductionDb) noexcept
    {
        const double desired = std::pow(10., -double(reductionDb) / 20.);
        return std::clamp((1. / std::max(desired, 1.e-6) - 1.) / seriesResistance, 0., 4. * beta);
    }
    static float reduction(float requestedDb) noexcept
    {
        return float(20. * std::log10(1. + seriesResistance * conductance(requestedDb)));
    }
    static float process(float input, float requestedDb, float all) noexcept
    {
        const double x = double(input) * voltsPerUnit;
        const double g0 = conductance(requestedDb);
        const double linearisation = .47 - .12 * double(all);
        const double a = seriesResistance * beta * (2. * linearisation - 1.);
        const double b = 1. + seriesResistance * g0;
        double v;
        if (std::abs(a) < 1.e-12)
            v = x / b;
        else
        {
            const double discriminant = b * b + 4. * a * x;
            // Rationalized root avoids catastrophic cancellation at quiet levels.
            v = discriminant > 0 ? 2. * x / (b + std::sqrt(discriminant)) : -b / (2. * a);
        }
        // A shunt resistor cannot supply energy. Beyond the ohmic region,
        // constrain the solution to the passive divider's valid branch.
        v = std::clamp(v, std::min(0., x), std::max(0., x));
        return float(v / voltsPerUnit);
    }
};

class MagneticTransformer
{
    double h = 0, m = 0, low = 0;
    double decay = 0, fluxStep = 0, leakStep = 0;
    static void langevin(double x, double &value, double &derivative) noexcept
    {
        if (std::abs(x) >= .5)
        {
            const double coth = 1. / std::tanh(x);
            value = coth - 1. / x;
            derivative = 1. / (x * x) - coth * coth + 1.;
        }
        else
        {
            const double x2 = x * x;
            value = x * (1. / 3. - x2 / 45. + 2. * x2 * x2 / 945.);
            derivative = 1. / 3. - x2 / 15. + 2. * x2 * x2 / 189.;
        }
    }
    static double slope(double magnetisation, double field, double direction) noexcept
    {
        constexpr double shape = .063, pinning = .040, reversible = .17, coupling = .0016, asymmetry = .3;
        const double raw = (field + coupling * magnetisation) / shape;
        const double effective = 1. + asymmetry * std::tanh(std::clamp(raw, -40., 40.));
        const double q = raw / effective;
        double anhysteretic, derivative;
        langevin(q, anhysteretic, derivative);
        derivative /= (shape * effective);
        const double diff = anhysteretic - magnetisation;
        const double delta = direction >= 0 ? 1. : -1.;
        const double gate = ((delta > 0) == (diff > 0)) ? 1. : 0.;
        double denominator = (1. - reversible) * pinning * delta - coupling * diff;
        if (std::abs(denominator) < 1.e-6)
            denominator = std::copysign(1.e-6, denominator);
        const double irreversible = (1. - reversible) * gate * diff / denominator;
        double scale = 1. - reversible * coupling * derivative;
        if (std::abs(scale) < 1.e-6)
            scale = std::copysign(1.e-6, scale);
        const double result = (irreversible + reversible * derivative) / scale;
        return std::isfinite(result) ? std::clamp(result, 0., 100.) : 0.;
    }

  public:
    void prepare(double fs) noexcept
    {
        constexpr double twoPi = 6.2831853071795864769;
        decay = std::exp(-twoPi * 3. / fs);
        fluxStep = .2 * twoPi * 50. / fs;
        leakStep = -std::expm1(-twoPi * 32000. / fs);
        h = m = low = 0;
    }
    float process(float input) noexcept
    {
        // Voltage-driven shunt core, no upstream plugin's +5.8dB fit trim.
        const double winding = double(input) - .5 * h;
        const double flux = winding * fluxStep;
        const double permeability = slope(m, h, flux);
        const double dh = flux / (1. + permeability);
        h = std::clamp((h + dh) * decay, -8., 8.);
        m = std::clamp(m + permeability * dh, -4., 4.);
        low += leakStep * (winding - low);
        if (!std::isfinite(low))
        {
            h = m = low = 0;
        }
        return float(low);
    }
};
} // namespace assassin
