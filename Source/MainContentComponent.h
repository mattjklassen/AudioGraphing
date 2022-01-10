

#pragma once

#include <string>
#include "../JuceLibraryCode/JuceHeader.h"
#include "GraphComponent.h"


//==============================================================================
class MainContentComponent   : public juce::AudioAppComponent,
                               public juce::ChangeListener,
                               public juce::ScrollBar::Listener,
                               public juce::Value::Listener,
                               public juce::Slider::Listener
{
public:
    
    MainContentComponent();

    ~MainContentComponent() override
    {
        shutdownAudio();
    }
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    void releaseResources() override
    {
        transportSource.releaseResources();
    }

    void resized() override;

    void changeListenerCallback (juce::ChangeBroadcaster* source) override;
    
    bool audioDataLoaded = false;

    Value lEP;  // left End Point for graphing interval in graphView
    Value rEP;  // right End Point for graphing interval in graphView

    
private:
    
    enum TransportState
    {
        Stopped, Starting, Playing, Pausing, Paused, Stopping
    };

    void scrollBarMoved (ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    
    void sliderValueChanged (juce::Slider* slider) override;
    
    void valueChanged (Value &value) override;

    void changeState (TransportState newState);

    void paint (juce::Graphics& g) override;

    int getMult(int numSamples);
    
    void setButtonColours();
    
    void drawScrollBox(juce::Graphics& g);
    
    void mouseDown (const MouseEvent& event) override
    {
        DBG ("Clicked at: " << event.getPosition().toString());
        std::cout << "lEP: " << lEP.toString() << "  rEP: " << rEP.toString() << std::endl;
    }
    
    void mouseDrag (const MouseEvent& event) override
    {
//        DBG ("Dragged at: " << event.getPosition().toString());
    }

    void openButtonClicked();

    void closeButtonClicked();
    
    void playButtonClicked();
    
    void stopButtonClicked();

    void graphButtonClicked();

    void readAudioData2 (AudioFormatReader *reader);
    
    bool keyStateChanged(bool isKeyDown) override;
    
    juce::TextButton openButton;
    juce::TextButton closeButton;
    juce::TextButton playButton;
    juce::TextButton stopButton;
    juce::TextButton graphButton;
    juce::ScrollBar signalScrollBar;
    
    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;
    juce::AudioBuffer<float> floatBuffer;

    juce::AudioFormatReader *reader;
    juce::SoundPlayer player;
    juce::AudioDeviceManager myADM;
    
    unsigned dataSize,          // size of data in bytes
             sampleRate,        // sample rate
             sampleCount;       // number of data samples
    float w = 0, h = 0;         // width and height of display for MainContentComponent
    float leftEndPoint = 0;     // left endpoint in (float)samples of Interval to graph
    float rightEndPoint = 2000; // right endpoint in (float)samples of Interval to graph
    float addoffset = 0;        // accumulate for left-right Interval shifting
    float magnify = 1;          // to scale the size of Interval
    float magfactor = 1;        // accumulate to give magnification
    int numSamples = 2000;      // for width of (displayed) graph time interval in samples
    int startIndex = 0;         // index of first cycle to graph
    int endIndex = 0;           // index of last cycle to graph
    Array<juce::Colour> buttonColours;
    juce::Point<int> doubleClick;
    GraphComponent graphView;

    float currentSampleRate = 0.0, currentAngle = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainContentComponent)
};

