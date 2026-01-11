#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "Panels.h"
#include "Editor.h"

//==============================================================================
class WavetableAudioProcessorEditor : public gin::ProcessorEditor,
                                      public juce::DragAndDropContainer
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
    WavetableAudioProcessor& wtProc;

    gin::TriggeredScope scope { wtProc.scopeFifo };
    gin::SynthesiserUsage usage { wtProc };
    gin::ModulationOverview modOverview { wtProc.modMatrix };
    gin::ModOverlay modOverlay;

    Editor editor { wtProc };

   #if JUCE_DEBUG
    std::unique_ptr<melatonin::Inspector> inspector;
   #endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WavetableAudioProcessorEditor)
};
