/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"


//==============================================================================
ShimmerVerbAudioProcessor::ShimmerVerbAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), parameters(*this, nullptr, "PARAMS", {

    std::make_unique<juce::AudioParameterFloat>("mix",      "Mix",       0.0f, 1.0f, 0.5f),
    std::make_unique<juce::AudioParameterBool>("shimmerOn","Shimmer",   true),
    std::make_unique<juce::AudioParameterFloat>("fb",       "Feedback",  0.0f, 0.35f, 0.25f),
    std::make_unique<juce::AudioParameterInt>("semiA",    "Pitch A",  -24, 24, 12),
    std::make_unique<juce::AudioParameterFloat>("gainA",    "Gain A",    0.0f, 1.0f, 0.7f),
    std::make_unique<juce::AudioParameterInt>("semiB",    "Pitch B",  -24, 24, 7),
    std::make_unique<juce::AudioParameterFloat>("gainB",    "Gain B",    0.0f, 1.0f, 0.3f),
    std::make_unique<juce::AudioParameterFloat>("toneHPF",  "Loop HPF",  50.0f, 600.0f, 250.0f),
    std::make_unique<juce::AudioParameterFloat>("toneLPF",  "Loop LPF", 3000.0f,12000.0f,8000.0f),
    
    std::make_unique<juce::AudioParameterBool>("onC",   "Pitch C On",    false),
    std::make_unique<juce::AudioParameterInt>("semiC", "Pitch C",  -24, 24, -12), // OctDown
    std::make_unique<juce::AudioParameterFloat>("gainC", "Gain C",   0.0f,  1.0f,  0.40f),

    std::make_unique<juce::AudioParameterBool>("onD",   "Pitch D On",    false),
    std::make_unique<juce::AudioParameterInt>("semiD","Pith D",  -24, 24,  4),  // Major 3rd
    std::make_unique<juce::AudioParameterFloat>("gainD","Gain D",   0.0f,  1.0f,  0.30f),
    // Zita core:
    std::make_unique<juce::AudioParameterFloat>("decay",   "Mid RT60", 1.0f, 8.0f, 3.0f),
    std::make_unique<juce::AudioParameterFloat>("lowRT60", "Low RT60", 1.0f, 8.0f, 3.0f),
    std::make_unique<juce::AudioParameterFloat>("hfDamp",  "HF Damp",  1500.f, 12000.f, 6000.f)




                           })
#endif
{
}

ShimmerVerbAudioProcessor::~ShimmerVerbAudioProcessor()
{
}

//==============================================================================
const juce::String ShimmerVerbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ShimmerVerbAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ShimmerVerbAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ShimmerVerbAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ShimmerVerbAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ShimmerVerbAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ShimmerVerbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ShimmerVerbAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ShimmerVerbAudioProcessor::getProgramName (int index)
{
    return {};
}

void ShimmerVerbAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ShimmerVerbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int ch = juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());

    // Zita
    zita.init((int)sampleRate);
    zita.instanceClear();
    zita.setWetDry(-1.0f);
    zita.setOutputLevel(0.0f);

    // ‌Buffers
    wetBuf.setSize(ch, samplesPerBlock);
    dryBuf.setSize(ch, samplesPerBlock);
    fbBuf.setSize(ch, samplesPerBlock);
    wetBuf.clear(); dryBuf.clear(); fbBuf.clear();
    fbInit = false;

    
    const int grainMsOctUp = 70;  // Layer A (OctUp)
    const int grainMsFifth = 60;  // Layer B (Fifth)
    const int grainMsOctDn = 90;  // Layer C (OctDown)
    const int grainMsThird = 60;  // Layer D (Third)

   
    layers[0].enabled = true;  // OctUp 
    layers[1].enabled = false; // Fifth
    layers[2].enabled = false; // OctDown
    layers[3].enabled = false; // Third

    
    layers[0].shifter.prepare(sampleRate, ch, samplesPerBlock, grainMsOctUp);
    layers[1].shifter.prepare(sampleRate, ch, samplesPerBlock, grainMsFifth);
    layers[2].shifter.prepare(sampleRate, ch, samplesPerBlock, grainMsOctDn);
    layers[3].shifter.prepare(sampleRate, ch, samplesPerBlock, grainMsThird);



    juce::dsp::ProcessSpec spec{ sampleRate, (juce::uint32)samplesPerBlock, (juce::uint32)ch };

    loopHPF.prepare(spec);
    loopLPF.prepare(spec);

    *loopHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, 250.0f);
    *loopLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0f);

    //  Latency compensation
    setLatencySamples(2048 / 8); // fftSize / overlap
}



void ShimmerVerbAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ShimmerVerbAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


void ShimmerVerbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int n = buffer.getNumSamples();
    const int ch = juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels());

    
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear(c, 0, n);

    // --- Params ----
    const float mix = parameters.getRawParameterValue("mix")->load();
    const bool  shimOn = parameters.getRawParameterValue("shimmerOn")->load();
    const float fbAmt = parameters.getRawParameterValue("fb")->load();

    const float semiA = parameters.getRawParameterValue("semiA")->load();
    const float gainA = parameters.getRawParameterValue("gainA")->load();
    const float semiB = parameters.getRawParameterValue("semiB")->load();
    const float gainB = parameters.getRawParameterValue("gainB")->load();

    const bool  onC = parameters.getRawParameterValue("onC")->load();
    const float semiC = parameters.getRawParameterValue("semiC")->load();
    const float gainC = parameters.getRawParameterValue("gainC")->load();
    const bool  onD = parameters.getRawParameterValue("onD")->load();
    const float semiD = parameters.getRawParameterValue("semiD")->load();
    const float gainD = parameters.getRawParameterValue("gainD")->load();

    const float hpfHz = parameters.getRawParameterValue("toneHPF")->load();
    const float lpfHz = parameters.getRawParameterValue("toneLPF")->load();

    const float decay = parameters.getRawParameterValue("decay")->load();
    const float lowRT60 = parameters.getRawParameterValue("lowRT60")->load();
    const float hfDamp = parameters.getRawParameterValue("hfDamp")->load();

	// --- Parameters to layers ---
    layers[0].enabled = true;  layers[0].semitones = semiA; layers[0].gain = gainA; // A (OctUp)
    layers[1].enabled = true;  layers[1].semitones = semiB; layers[1].gain = gainB; // B (Fifth)
    layers[2].enabled = onC;   layers[2].semitones = semiC; layers[2].gain = gainC; // C (OctDown)
    layers[3].enabled = onD;   layers[3].semitones = semiD; layers[3].gain = gainD; // D (Third)

    for (auto& L : layers)
        L.shifter.setSemitones(L.semitones);

    // --- Dry/Wet buffers ---
    dryBuf.setSize(ch, n, false, false, true);
    wetBuf.setSize(ch, n, false, false, true);
    fbBuf.setSize(ch, n, false, false, true);

    dryBuf.clear(); wetBuf.clear();

    for (int c = 0; c < juce::jmin(ch, buffer.getNumChannels()); ++c)
        dryBuf.copyFrom(c, 0, buffer, c, 0, n);

    // --- Zita Reverb ---
    zita.setWetDry(-1.0f);
    zita.setOutputLevel(0.0f);
    zita.setDecay(decay);
    zita.setLowRT60(lowRT60);
    zita.setHFDamping(hfDamp);

    
    juce::AudioBuffer<float> inToZita(dryBuf);
    if (fbInit && shimOn)
        for (int c = 0; c < ch; ++c)
            inToZita.addFrom(c, 0, fbBuf, c, 0, n, juce::jlimit(0.0f, 0.9f, fbAmt));

    {
        std::vector<float*> inPtrs(ch), outPtrs(ch);
        for (int c = 0; c < ch; ++c) {
            inPtrs[c] = inToZita.getWritePointer(c);
            outPtrs[c] = wetBuf.getWritePointer(c);
        }
        zita.compute(n, inPtrs.data(), outPtrs.data());
    }

    // --- Shimmer feedback loop
    if (shimOn)
    {
      
        juce::AudioBuffer<float> loopIn(wetBuf);

       
        *loopHPF.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(getSampleRate(), hpfHz);
        *loopLPF.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), lpfHz);

        {
            juce::dsp::AudioBlock<float> ab(loopIn);
            juce::dsp::ProcessContextReplacing<float> ctx(ab);
            loopHPF.process(ctx);
            loopLPF.process(ctx);
        }

     
        fbBuf.clear();

        for (int li = 0; li < 4; ++li)
        {
            if (!layers[li].enabled) continue;

            juce::AudioBuffer<float> tmp; 
            tmp.setSize(ch, n, false, false, true);
            tmp.clear();

           
            layers[li].shifter.setSemitones(layers[li].semitones);
            layers[li].shifter.processBlock(loopIn, tmp);

            for (int c = 0; c < ch; ++c)
                fbBuf.addFrom(c, 0, tmp, c, 0, n, layers[li].gain);
        }

     
        fbBuf.applyGain(0.85f);
        for (int c = 0; c < ch; ++c) {
            auto* d = fbBuf.getWritePointer(c);
            for (int i = 0; i < n; ++i)
                d[i] = std::tanh(d[i] * 1.5f);
        }

        fbInit = true;
    }
    else
    {
        fbBuf.clear();
        fbInit = false;
    }

    // --- Final Dry/Wet Mix ---
    for (int c = 0; c < ch; ++c)
    {
        const float* dry = dryBuf.getReadPointer(c);
        const float* wet = wetBuf.getReadPointer(c);
        float* out = buffer.getWritePointer(c);

        for (int i = 0; i < n; ++i)
            out[i] = dry[i] * (1.0f - mix) + wet[i] * mix;
    }
}




//==============================================================================
bool ShimmerVerbAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ShimmerVerbAudioProcessor::createEditor()
{
    //return new ShimmerVerbAudioProcessorEditor (*this);
    //return new juce::GenericAudioProcessorEditor(*this);
    auto* editor = new juce::GenericAudioProcessorEditor(*this);
    editor->setSize(500, 750); 
    return editor;
}

//==============================================================================
void ShimmerVerbAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ShimmerVerbAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ShimmerVerbAudioProcessor();
}
