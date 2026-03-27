#include "VCACompressor.h"

VCACompressor::VCACompressor() {}

void VCACompressor::prepare (double sampleRate)
{
    sr = sampleRate;
    reset();
}

void VCACompressor::reset()
{
    smoothedGrDb = 0.0;
    dualFastEnv = 0.0;
    dualSlowEnv = 0.0;
    wasCompressing = false;
    currentGrDb = 0.0f;
}

double VCACompressor::makeCoeff (double timeSec) const
{
    if (timeSec <= 0.0) return 0.0;
    return std::exp (-1.0 / (sr * timeSec));
}

float VCACompressor::computeGainReduction (float inputDb, float thresholdDb, float ratio) const
{
    // Soft-knee gain computer
    float halfKnee = kneeWidthDb * 0.5f;

    if (inputDb < thresholdDb - halfKnee)
    {
        return 0.0f;  // below threshold — no compression
    }
    else if (inputDb > thresholdDb + halfKnee)
    {
        // Above knee — full compression
        float compressed = thresholdDb + (inputDb - thresholdDb) / ratio;
        return compressed - inputDb;  // negative dB value
    }
    else
    {
        // In knee region — quadratic interpolation
        float x = inputDb - thresholdDb + halfKnee;
        return ((1.0f / ratio) - 1.0f) * x * x / (2.0f * kneeWidthDb);
    }
}

float VCACompressor::processSample (float sidechainDb, float thresholdDb, float ratio,
                                     float attackSec, float releaseSec, bool dualRelease)
{
    // Compute desired gain reduction
    float desiredGrDb = computeGainReduction (sidechainDb, thresholdDb, ratio);

    if (! dualRelease)
    {
        // Standard attack/release smoothing
        double coeff;
        if (desiredGrDb < smoothedGrDb)
            coeff = makeCoeff ((double) attackSec);   // attacking (GR going more negative)
        else
            coeff = makeCoeff ((double) releaseSec);  // releasing

        smoothedGrDb = coeff * smoothedGrDb + (1.0 - coeff) * (double) desiredGrDb;
    }
    else
    {
        // Dual release mode: two-stage release mimicking optical behavior
        double attackCoeff = makeCoeff ((double) attackSec);

        if (desiredGrDb < smoothedGrDb)
        {
            // Attacking
            smoothedGrDb = attackCoeff * smoothedGrDb + (1.0 - attackCoeff) * (double) desiredGrDb;
            dualFastEnv = smoothedGrDb;
            dualSlowEnv = smoothedGrDb;
            wasCompressing = true;
        }
        else
        {
            // Releasing: blend fast (~60ms) and slow (~2s) release
            double fastCoeff = makeCoeff (0.06);   // 60ms — first 50-80% of recovery
            double slowCoeff = makeCoeff (2.0);    // 2s — remaining recovery

            dualFastEnv = fastCoeff * dualFastEnv + (1.0 - fastCoeff) * (double) desiredGrDb;
            dualSlowEnv = slowCoeff * dualSlowEnv + (1.0 - slowCoeff) * (double) desiredGrDb;

            // Blend: use the DEEPER (more negative) of the two
            // This naturally creates the two-stage behavior:
            // fast env recovers quickly, slow env holds longer
            smoothedGrDb = std::min (dualFastEnv, dualSlowEnv);
            wasCompressing = false;
        }
    }

    currentGrDb = (float) smoothedGrDb;

    // Convert GR dB to linear gain
    return std::pow (10.0f, currentGrDb / 20.0f);
}
