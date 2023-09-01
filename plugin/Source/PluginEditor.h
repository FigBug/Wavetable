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
    void addMenuItems (juce::PopupMenu& m) override;

    void showAboutInfo() override;

private:
    WavetableAudioProcessor& vaProc;

    gin::TriggeredScope scope { vaProc.scopeFifo };
    Editor editor { vaProc };

   #if JUCE_DEBUG
    std::unique_ptr<melatonin::Inspector> inspector;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
