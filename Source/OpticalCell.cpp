#include "OpticalCell.h"

OpticalCell::OpticalCell() {}

void OpticalCell::prepare (double sampleRate)
{
    sr = sampleRate;
    dt = 1.0 / (2.0 * sampleRate);  // 2x oversampled
    reset();
}

void OpticalCell::reset()
{
    q_el = 0.0;
    R_cds = R_cds_dark;
    lightOutput = 0.0;
    lightHistory = 0.0;
    currentGain = 1.0;
    prevInput = 0.0f;
}

void OpticalCell::solveImplicitStep (double inputLevel)
{
    // Drive from sidechain level (rectified, normalized 0..1+)
    double i_drive = std::max (0.0, inputLevel);

    // Store old state for trapezoidal rule
    double q_old = q_el;
    double f_old = (i_drive - q_old / C_el) / R_el;

    // Initial guess (forward Euler)
    double q_new = q_old + dt * f_old;

    // Newton-Raphson iteration (implicit trapezoidal)
    for (int iter = 0; iter < maxNewtonIter; ++iter)
    {
        double f_new = (i_drive - q_new / C_el) / R_el;
        double residual = q_new - q_old - (dt / 2.0) * (f_old + f_new);
        double jacobian = 1.0 + dt / (2.0 * R_el * C_el);

        double delta = residual / jacobian;
        q_new -= delta;

        if (std::abs (delta) < newtonTol)
            break;
    }

    q_el = q_new;

    // Light output proportional to STORED ENERGY (voltage^2)
    // This ensures continuous light output at steady state
    double voltage = q_el / C_el;
    lightOutput = eta_el * voltage * voltage;

    // Update memory (illumination history) — explicit Euler
    lightHistory += dt * (lightOutput - lightHistory) / memoryTau;
    lightHistory = std::max (0.0, lightHistory);

    // CdS resistance: R = k * (L_eff)^(-gamma)
    // Memory effect: effective light includes accumulated history
    double L_eff = lightOutput + memoryAlpha * lightHistory;
    double epsilon = 1.0e-10;

    R_cds = k_cds * std::pow (L_eff + epsilon, -gamma);
    R_cds = std::clamp (R_cds, R_cds_min, R_cds_dark);
}

void OpticalCell::updateGain()
{
    // Shunt attenuator: high R_cds = pass, low R_cds = attenuate
    currentGain = R_cds / (R_fixed + R_cds);
}

float OpticalCell::processSample (float sidechainLevel, float thresholdLinear)
{
    // Envelope above threshold
    float envelope = std::max (0.0f, sidechainLevel - thresholdLinear);

    // 2x oversampling: interpolate and process two sub-samples
    float mid = (prevInput + envelope) * 0.5f;
    prevInput = envelope;

    solveImplicitStep ((double) mid);
    solveImplicitStep ((double) envelope);

    updateGain();

    return (float) currentGain;
}

float OpticalCell::getGainReductionDb() const
{
    if (currentGain <= 0.0)
        return -60.0f;
    return (float) (20.0 * std::log10 (currentGain));
}
