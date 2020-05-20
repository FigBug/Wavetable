#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Boxes.h"

//==============================================================================
class WavetableAudioProcessorEditor : public gin::ProcessorEditor
{
public:
    WavetableAudioProcessorEditor (WavetableAudioProcessor&);
    ~WavetableAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

private:
    WavetableAudioProcessor& proc;

    gin::ModulationOverview modOverview { proc.modMatrix };

    gin::ControlHeader commonHeader { "Common" };
    gin::ControlHeader unisonHeader { "Unison" };

    CommonBox common { *this, proc };

    gin::ControlHeader oscHeaders = { "Oscillators" };
    OscillatorBox oscillators     = { *this, proc };

    gin::ControlHeader ampFiltersHeader { "Filter / ADSR" };
    FilterAmpBox ampFilters { *this, proc };

    gin::ControlHeader modulationHeader { "Modulation" };
    ModulationBox modulation { *this, proc };

    gin::ControlHeader effectsHeader { "Effects" };
    EffectsBox effects { *this, proc };

    gin::SynthesiserUsage usage { proc };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
