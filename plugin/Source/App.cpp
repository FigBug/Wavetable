#include <JuceHeader.h>
#include "PluginProcessor.h"

static void profile (juce::PropertiesFile& settings)
{
    WavetableAudioProcessor proc;
    proc.setRateAndBufferSizeDetails (44100, 512);
    proc.prepareToPlay (44100, 512);
    
    juce::MemoryBlock data;

    if (data.fromBase64Encoding (settings.getValue ("filterState")) && data.getSize() > 0)
        proc.setStateInformation (data.getData(), (int) data.getSize());

    proc.reset();
    
    juce::AudioSampleBuffer buffer (2, 512);
    
    for (int i = 0; i < 5000; i++)
    {
        buffer.clear();
        
        if (i % 100 == 0)
        {
            juce::MidiBuffer midiOn;
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 60, 0.5f), 0);
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 70, 0.5f), 0);
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 80, 0.5f), 0);
            
            proc.processBlock (buffer, midiOn);
        }
        else if (i % 100 == 90)
        {
            juce::MidiBuffer midiOff;

            midiOff.addEvent (juce::MidiMessage::noteOff (1, 60, 0.5f), 0);
            midiOff.addEvent (juce::MidiMessage::noteOff (1, 70, 0.5f), 0);
            midiOff.addEvent (juce::MidiMessage::noteOff (1, 80, 0.5f), 0);
            
            proc.processBlock (buffer, midiOff);
        }
        else
        {
            juce::MidiBuffer midiEmpty;
            proc.processBlock (buffer, midiEmpty);
        }
    }

}

juce::JUCEApplicationBase* juce_CreateApplication()
{
    return new gin::StandaloneApp([] (juce::PropertiesFile& settings)
    {
        if (juce::JUCEApplication::getCommandLineParameters ().contains ("-profile"))
        {
            auto start = juce::Time::getMillisecondCounterHiRes();
            profile (settings);
            auto end = juce::Time::getMillisecondCounterHiRes();
            
            printf ("Elapsed time: %.2fs\n", (end - start) / 1000);
                
            return false;
        }
        return true;
    });
}
