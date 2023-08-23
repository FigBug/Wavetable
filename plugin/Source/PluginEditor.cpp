#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WavetableAudioProcessorEditor::WavetableAudioProcessorEditor (WavetableAudioProcessor& p)
    : ProcessorEditor (p), vaProc (p)
{
    addAndMakeVisible (editor);

    setSize (901, 753);
}

WavetableAudioProcessorEditor::~WavetableAudioProcessorEditor()
{
}

//==============================================================================
void WavetableAudioProcessorEditor::paint (juce::Graphics& g)
{
    ProcessorEditor::paint (g);

    titleBar.setShowBrowser (true);

    g.fillAll (findColour (gin::PluginLookAndFeel::blackColourId));
}

void WavetableAudioProcessorEditor::resized()
{
    ProcessorEditor::resized ();

    auto rc = getLocalBounds().reduced (1);
    rc.removeFromTop (40);

    editor.setBounds (rc);
}
