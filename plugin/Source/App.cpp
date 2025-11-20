#include <JuceHeader.h>
#include "PluginProcessor.h"

#if 0

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
    
    juce::MidiBuffer midiOn;
    juce::MidiBuffer midiOff;
    juce::MidiBuffer midiEmpty;
    
    auto start = juce::Time::getMillisecondCounterHiRes();
    
    for (int i = 0; i < 20000; i++)
    {
        buffer.clear();
        
        if (i % 100 == 0)
        {
            midiOn.clear();
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 60, 0.5f), 0);
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 70, 0.5f), 0);
            midiOn.addEvent (juce::MidiMessage::noteOn (1, 80, 0.5f), 0);
            
            proc.processBlock (buffer, midiOn);
        }
        else if (i % 100 == 90)
        {
            midiOff.clear();
            midiOff.addEvent (juce::MidiMessage::noteOff (1, 60, 0.5f), 0);
            midiOff.addEvent (juce::MidiMessage::noteOff (1, 70, 0.5f), 0);
            midiOff.addEvent (juce::MidiMessage::noteOff (1, 80, 0.5f), 0);
            
            proc.processBlock (buffer, midiOff);
        }
        else
        {
            midiEmpty.clear();
            proc.processBlock (buffer, midiEmpty);
        }
    }
    
    auto end = juce::Time::getMillisecondCounterHiRes();
    printf ("Elapsed time: %.2fs\n", (end - start) / 1000);
}

#endif
