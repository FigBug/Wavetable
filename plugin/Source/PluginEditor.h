#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Panels.h"
#include "Editor.h"

//==============================================================================
class WavetableAudioProcessorEditor : public gin::ProcessorEditor
{
public:
    WavetableAudioProcessorEditor (WavetableAudioProcessor&);
    ~WavetableAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    WavetableAudioProcessor& vaProc;

    Editor editor { vaProc };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
