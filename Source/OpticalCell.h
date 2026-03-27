#pragma once
#include <cmath>
#include <algorithm>

// ============================================================
// Port-Hamiltonian T4B Electro-Optical Compressor Model
//
// Models the EL panel (capacitive energy store) coupled to a
// CdS photoresistor (nonlinear dissipation) using implicit
// trapezoidal integration with Newton-Raphson iteration.
//
// All parameters are normalized for audio-level (0..1) operation.
// ============================================================
class OpticalCell
{
public:
    OpticalCell();

    void prepare (double sampleRate);
    void reset();

    // Process one sample: sidechain input (linear), returns gain (0..1)
    float processSample (float sidechainLevel, float thresholdLinear);

    // Current GR in dB (for metering, always <= 0)
    float getGainReductionDb() const;

private:
    double sr = 44100.0;
    double dt = 1.0 / 88200.0;  // 2x oversampled time step

    // ---- Port-Hamiltonian State (normalized units) ----

    // EL panel: capacitive energy store
    double q_el = 0.0;          // charge state
    double C_el = 0.01;         // capacitance (~10ms attack with R_el=1)
    double R_el = 1.0;          // series resistance (normalized)
    double eta_el = 50.0;       // electro-optical efficiency (scaled for audio levels)

    // CdS photoresistor: nonlinear dissipation
    double R_cds = 100.0;       // current resistance (normalized)
    double R_cds_dark = 100.0;  // dark resistance (no light → no compression)
    double R_cds_min = 0.01;    // minimum resistance (max compression)
    double k_cds = 1.0;         // CdS scaling factor
    double gamma = 0.7;         // CdS nonlinearity exponent

    // Memory effect (CdS illumination history)
    double lightHistory = 0.0;
    double memoryTau = 2.0;     // memory time constant (seconds)
    double memoryAlpha = 0.3;   // history influence factor

    // Light state
    double lightOutput = 0.0;

    // Signal path impedance for gain (normalized)
    double R_fixed = 1.0;

    // Current gain (linear)
    double currentGain = 1.0;

    // Implicit solver
    static constexpr int maxNewtonIter = 5;
    static constexpr double newtonTol = 1.0e-8;

    // 2x oversampling
    float prevInput = 0.0f;

    // Core methods
    void solveImplicitStep (double inputLevel);
    void updateGain();
};
