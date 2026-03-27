#pragma once
#include <cmath>
#include <algorithm>

class VCACompressor
{
public:
    VCACompressor();

    void prepare (double sampleRate);
    void reset();

    // Process one sample: sidechainDb = input level in dB
    // Returns gain in linear scale (0..1)
    float processSample (float sidechainDb, float thresholdDb, float ratio,
                         float attackSec, float releaseSec, bool dualRelease);

    float getGainReductionDb() const { return currentGrDb; }

private:
    double sr = 44100.0;

    // Gain smoothing state (dB domain)
    double smoothedGrDb = 0.0;

    // Dual release state
    double dualFastEnv = 0.0;    // fast release envelope
    double dualSlowEnv = 0.0;    // slow release envelope
    bool wasCompressing = false;

    float currentGrDb = 0.0f;

    // Soft-knee width
    static constexpr float kneeWidthDb = 6.0f;

    // Gain computer: returns desired GR in dB (negative value)
    float computeGainReduction (float inputDb, float thresholdDb, float ratio) const;

    // Coefficient helpers
    double makeCoeff (double timeSec) const;
};
