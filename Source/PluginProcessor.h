/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>
#include "ZitaRev.cpp" 
#include "GranularPitchShift.h"



class ShimmerVerbAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    ShimmerVerbAudioProcessor();
    ~ShimmerVerbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
	//==============================================================================
    struct ShimmerLayer
    {
        bool enabled = false;
        float semitones = 0.0f;
        float gain = 0.0f;
        GranularPitchShift shifter;  
    };


private:

	// Parameters and Buffers
    juce::AudioProcessorValueTreeState parameters;
    juce::AudioBuffer<float> dryBuf, wetBuf, prevWetBuf;
    bool prevWetInitialised = false;

    // Shimmer state
    juce::AudioBuffer<float> fbBuf;      
    bool fbInit = false;
    ShimmerLayer layers[4];  // A, B, C, D

	// Filters
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> loopHPF, loopLPF;

    // Zita
    ZitaRev zita;
   
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ShimmerVerbAudioProcessor)
};
