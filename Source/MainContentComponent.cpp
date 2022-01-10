/*
  ==============================================================================

    AudioWithSplineModel.cpp
    Created: 15 Nov 2021 9:04:57am
    Author:  Matt Klassen

  ==============================================================================
*/

#include "MainContentComponent.h"

MainContentComponent::MainContentComponent()
    : lEP(0.0), rEP(1200.0), signalScrollBar(false), state (Stopped)
{
    setButtonColours();
    
    addAndMakeVisible (&openButton);
    openButton.setButtonText ("Open");
    openButton.setColour (juce::TextButton::buttonColourId, buttonColours[0]);
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible (&closeButton);
    closeButton.setButtonText ("Close");
    closeButton.setColour (juce::TextButton::buttonColourId, buttonColours[3]);
    closeButton.onClick = [this] { closeButtonClicked(); };
    
    addAndMakeVisible (&playButton);
    playButton.setButtonText ("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour (juce::TextButton::buttonColourId, buttonColours[1]);
    playButton.setEnabled (false);

    addAndMakeVisible (&stopButton);
    stopButton.setButtonText ("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour (juce::TextButton::buttonColourId, buttonColours[2]);
    stopButton.setEnabled (false);
    
    addAndMakeVisible (&graphButton);
    graphButton.setButtonText ("Graph Signal");
    graphButton.onClick = [this] { graphButtonClicked(); };
    graphButton.setColour (juce::TextButton::buttonColourId, buttonColours[3]);

    addAndMakeVisible (&signalScrollBar);
    signalScrollBar.setVisible(true);
    signalScrollBar.setRangeLimits(0, 1000, sendNotificationAsync);
    signalScrollBar.addListener(this);
    addAndMakeVisible(&graphView);
    
    lEP.referTo(graphView.leftEP);
    rEP.referTo(graphView.rightEP);
    
    setSize (1200, 800);

    formatManager.registerBasicFormats();
    transportSource.addChangeListener (this);
    
    lEP.addListener (this);
    rEP.addListener (this);

    setAudioChannels (0, 2);
    
    myADM.initialise(0, 2, nullptr, true);
    myADM.addAudioCallback(&player);
//    player.playTestSound();
}

void MainContentComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::lightgrey);
    g.setColour (juce::Colours::blue);
    w = getWidth(); h = getHeight();
    
    drawScrollBox(g);
}

void MainContentComponent::setButtonColours()
{
    buttonColours.add (juce::Colour::fromHSV (0.5f, 0.5f, 0.7f, 0.6f));
    buttonColours.add (juce::Colour::fromHSV (0.7f, 0.7f, 0.7f, 0.6f));
    buttonColours.add (juce::Colour::fromHSV (0.6f, 0.5f, 0.5f, 0.6f));
    buttonColours.add (juce::Colour::fromHSV (0.8f, 0.5f, 0.7f, 0.6f));
    buttonColours.add (juce::Colour::fromHSV (0.6f, 0.3f, 0.4f, 0.6f));
}

void MainContentComponent::changeListenerCallback (juce::ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState (Playing);
        else if ((state == Stopping) || (state == Playing))
            changeState (Stopped);
        else if (Pausing == state)
            changeState (Paused);
    }
}

void MainContentComponent::resized()
{
    float w = getWidth();
    float h = getHeight();
    openButton.setBounds (10, 10, 70, 20);
    closeButton.setBounds (10, 70, 70, 20);
    playButton.setBounds (90, 10, 70, 20);
    stopButton.setBounds (170, 10, 70, 20);
    graphButton.setBounds (250, 10, 100, 20);

    signalScrollBar.setBounds (15, h-35, w-30, 20);
    graphView.setBounds (10, 100, w-20, h-145);

}

void MainContentComponent::readAudioData2 (AudioFormatReader *reader) {
    
    sampleCount = (int) reader->lengthInSamples;
    sampleRate = (int) reader->sampleRate;
    floatBuffer.setSize ((int) reader->numChannels, (int) reader->lengthInSamples);
    reader->read(&floatBuffer,                     // juce AudioBuffer <float>
                 0,                               // start sample in buffer
                 (int) reader->lengthInSamples,   // number of samples in file data
                 0,                               // start sample to fill in buffer
                 true,                            // use Left channel (0)
                 false);                          // use Right channel (1)
}

void MainContentComponent::graphButtonClicked()
{
    if (graphView.audioLoaded) {
        if (graphView.updateGraph) {
            graphView.updateGraph = false;
        } else {
            graphView.updateGraph = true;
        }
    }
    graphView.repaint();
}

void MainContentComponent::stopButtonClicked()
{
    if (state == Paused)
        changeState (Stopped);
    else
        changeState (Stopping);
}

void MainContentComponent::playButtonClicked()
{
    if ((state == Stopped) || (state == Paused))
        changeState (Starting);
    else if (state == Playing)
        changeState (Pausing);
}

void MainContentComponent::closeButtonClicked()
{
    graphView.updateGraph = false;
    graphView.audioLoaded = false;
    graphView.repaint();
}

void MainContentComponent::openButtonClicked()
{
    chooser = std::make_unique<juce::FileChooser> ("Select a Wave file to play...",
                                                   juce::File{},
                                                   "*.wav");
    auto chooserFlags = juce::FileBrowserComponent::openMode
                      | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync (chooserFlags, [this] (const FileChooser& fc)
    {
        auto file = fc.getResult();

        if (file != File{})                                                      
        {
            reader = formatManager.createReaderFor (file);  // file is InputStream
            readAudioData2(reader);
            audioDataLoaded = true;
            graphView.setDataForGraph(floatBuffer, audioDataLoaded, numSamples, magnify,
                                      leftEndPoint, rightEndPoint, sampleCount, sampleRate);
        
            // setting up to play back audio file (as in JUCE tutorial PlayingSoundFilesTutorial)
            if (reader != nullptr)
            {
                auto newSource = std::make_unique<juce::AudioFormatReaderSource> (reader, true);
                transportSource.setSource (newSource.get(), 0, nullptr, reader->sampleRate);
                playButton.setEnabled (true);
                prepareToPlay(512, sampleRate);
                readerSource.reset (newSource.release());
            }
        }
    });

}

void MainContentComponent::drawScrollBox(juce::Graphics& g)
{
    juce::Path path;
    path.startNewSubPath (juce::Point<float> (10, h-40));
    path.lineTo (juce::Point<float> (w-10, h-40));
    path.lineTo (juce::Point<float> (w-10, h-10));
    path.lineTo (juce::Point<float> (10, h-10));
    path.lineTo (juce::Point<float> (10, h-40));
    float myThickness = 1.0;
    juce::PathStrokeType myType = PathStrokeType(myThickness);
    g.strokePath (path, myType);
}

void MainContentComponent::scrollBarMoved (ScrollBar* scrollBarThatHasMoved ,
                                 double newRangeStart)
{
    double newRangeL = scrollBarThatHasMoved->getCurrentRange().getStart();
    graphView.leftEndPoint = (float) newRangeL / 1000 * (float) graphView.sampleCount;
    graphView.rightEndPoint = graphView.leftEndPoint + (float) graphView.numSamples;
    if (graphView.rightEndPoint > graphView.sampleCount)
    {
        graphView.rightEndPoint = (float) graphView.sampleCount;
        graphView.leftEndPoint = graphView.rightEndPoint - (float) graphView.numSamples;
    }
    if (newRangeL == 0) {
        graphView.hardLeft = true;
    }
    graphView.repaint();
}

void MainContentComponent::sliderValueChanged (juce::Slider* slider)
{
    if (! audioDataLoaded) {
        return;
    }
}

void MainContentComponent::valueChanged (Value &value)
{
    if ((value == lEP) || (value == rEP)) {
        // graph interval has changed, so need to change scrollBar size and/or position
        double newStart = ((double)lEP.getValue() / (double) sampleCount) * 1000;
        double newEnd =  ((double)rEP.getValue() / (double) sampleCount) * 1000;
        double newSize = newEnd - newStart;
        signalScrollBar.setCurrentRange(newStart, newSize, dontSendNotification);
        if ((int) newStart == 0) {
            graphView.hardLeft = true;
        }
        repaint();
    }
}

void MainContentComponent::changeState (TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
            case Stopped:
                playButton.setButtonText ("Play");
                stopButton.setButtonText ("Stop");
                stopButton.setEnabled (false);
                transportSource.setPosition (0.0);
                break;

            case Starting:
                transportSource.start();
                break;

            case Playing:
                playButton.setButtonText ("Pause");
                stopButton.setButtonText ("Stop");
                stopButton.setEnabled (true);
                break;

            case Pausing:
                transportSource.stop();
                break;

            case Paused:
                playButton.setButtonText ("Resume");
                stopButton.setButtonText ("Return to Zero");
                break;

            case Stopping:
                transportSource.stop();
                break;
        }
    }
}

void MainContentComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    transportSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}


void MainContentComponent::getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill)
{
        if (readerSource.get() == nullptr)
        {
            bufferToFill.clearActiveBufferRegion();
            return;
        }
        transportSource.getNextAudioBlock (bufferToFill);
}

bool MainContentComponent::keyStateChanged(bool isKeyDown)
{
    KeyPress cmd1 = KeyPress('1', ModifierKeys::commandModifier, 0);
    KeyPress cmd3 = KeyPress('3', ModifierKeys::commandModifier, 0);
    float incr = 0.001;
    if (KeyPress::isKeyCurrentlyDown(KeyPress::leftKey) || cmd1.isCurrentlyDown())
    {
        DBG("leftKey DOWN");
        graphView.magnify = 1.1 + incr;
        graphView.repaint();
    }
    if (KeyPress::isKeyCurrentlyDown(KeyPress::rightKey) || cmd3.isCurrentlyDown())
    {
        DBG("rightKey DOWN");
        graphView.magnify = 0.9 - incr;
        graphView.repaint();
    }
    return true;
}
