#include "CompressorEngine.h"

CompressorEngine::CompressorEngine() {}

void CompressorEngine::prepare (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    for (auto& cell : optoCells) cell.prepare (sampleRate);
    for (auto& comp : vcaComps) comp.prepare (sampleRate);
    transformer.prepare (sampleRate, samplesPerBlock);

    // Init sidechain HPF
    lastScHpfFreq = 0.0f;
    updateScHpf (90.0f);

    for (auto& f : scHpfFilters) f.reset();
}

void CompressorEngine::updateScHpf (float freqHz)
{
    if (std::abs (freqHz - lastScHpfFreq) < 0.1f) return;
    lastScHpfFreq = freqHz;

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass (currentSampleRate, freqHz);
    for (auto& f : scHpfFilters)
        f.coefficients = coeffs;
}

void CompressorEngine::processBlock (juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = std::min (buffer.getNumChannels(), 2);

    if (globalBypass.load() || numChannels == 0) return;

    // Measure input level BEFORE any processing
    inputLevelL.store (buffer.getMagnitude (0, 0, numSamples));
    if (numChannels > 1)
        inputLevelR.store (buffer.getMagnitude (1, 0, numSamples));
    else
        inputLevelR.store (inputLevelL.load());

    // Read parameters once per block
    float optoThresh = optoThresholdDb.load();
    float optoGain   = optoGainDb.load();
    float scHpf      = optoScHpfHz.load();
    bool  optoBp     = optoBypass.load();

    float vcaThresh  = vcaThresholdDb.load();
    float vcaGain    = vcaGainDb.load();
    int   ratioIdx   = std::clamp (vcaRatioIndex.load(), 0, numRatios - 1);
    int   attackIdx  = std::clamp (vcaAttackIndex.load(), 0, numAttacks - 1);
    int   releaseIdx = std::clamp (vcaReleaseIndex.load(), 0, numReleases - 1);
    bool  vcaBp      = vcaBypass.load();

    int   xfmrType   = transformerType.load();
    float outGain    = std::pow (10.0f, outputGainDb.load() / 20.0f);
    bool  linked     = stereoLink.load();

    float ratio      = ratios[ratioIdx];
    float attackSec  = attacks[attackIdx];
    float releaseSec = releases[releaseIdx];
    bool  dualRel    = (releaseSec < 0.0f);
    if (dualRel) releaseSec = 0.5f;  // fallback (not used in dual mode)

    float optoThreshLin = std::pow (10.0f, optoThresh / 20.0f);
    float optoGainLin   = std::pow (10.0f, optoGain / 20.0f);
    float vcaGainLin    = std::pow (10.0f, vcaGain / 20.0f);

    // Update sidechain HPF
    updateScHpf (scHpf);

    // Per-sample processing
    float peakOptoGr = 0.0f;
    float peakVcaGr = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        // Get input samples
        float inL = buffer.getSample (0, i);
        float inR = (numChannels > 1) ? buffer.getSample (1, i) : inL;

        // Sidechain: HPF filtered input for detection
        float scL = scHpfFilters[0].processSample (inL);
        float scR = (numChannels > 1) ? scHpfFilters[1].processSample (inR) : scL;

        // Sidechain level (rectified)
        float scLevelL = std::abs (scL);
        float scLevelR = std::abs (scR);

        if (linked)
        {
            float scMono = (scLevelL + scLevelR) * 0.5f;
            scLevelL = scMono;
            scLevelR = scMono;
        }

        // --- Optical compressor ---
        float optoGainL = 1.0f, optoGainR = 1.0f;
        if (! optoBp)
        {
            optoGainL = optoCells[0].processSample (scLevelL, optoThreshLin);
            optoGainR = linked ? optoGainL : optoCells[1].processSample (scLevelR, optoThreshLin);

            inL *= optoGainL * optoGainLin;
            inR *= optoGainR * optoGainLin;

            float gr = optoCells[0].getGainReductionDb();
            if (gr < peakOptoGr) peakOptoGr = gr;
        }

        // --- VCA compressor ---
        if (! vcaBp)
        {
            // Convert to dB for VCA
            float scDbL = 20.0f * std::log10 (std::max (1.0e-6f, std::abs (inL)));
            float scDbR = 20.0f * std::log10 (std::max (1.0e-6f, std::abs (inR)));

            if (linked) scDbL = scDbR = std::max (scDbL, scDbR);

            float vcaGainL = vcaComps[0].processSample (scDbL, vcaThresh, ratio, attackSec, releaseSec, dualRel);
            float vcaGainR = linked ? vcaGainL : vcaComps[1].processSample (scDbR, vcaThresh, ratio, attackSec, releaseSec, dualRel);

            inL *= vcaGainL * vcaGainLin;
            inR *= vcaGainR * vcaGainLin;

            float gr = vcaComps[0].getGainReductionDb();
            if (gr < peakVcaGr) peakVcaGr = gr;
        }

        // Write back (transformer and output gain applied later as block ops)
        buffer.setSample (0, i, inL);
        if (numChannels > 1) buffer.setSample (1, i, inR);
    }

    // --- Transformer saturation (block-level, 4x oversampled) ---
    if (xfmrType > 0 && xfmrType <= 3)
        transformer.processBlock (buffer, (TransformerSaturation::Type) xfmrType);

    // --- Output gain ---
    buffer.applyGain (outGain);

    // --- Metering ---
    optoGrDb.store (peakOptoGr);
    vcaGrDb.store (peakVcaGr);
    outputLevelL.store (buffer.getMagnitude (0, 0, numSamples));
    if (numChannels > 1)
        outputLevelR.store (buffer.getMagnitude (1, 0, numSamples));
    else
        outputLevelR.store (outputLevelL.load());
}
