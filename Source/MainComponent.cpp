#include "MainComponent.h"
#include <valarray>
#include <cmath>
#include "STN_utils.h"
#include <random>
#include <chrono>
#include "resampleComponent/resample.h"

//using namespace fft_utils;
using Vec2D = std::vector<std::vector<float>>;
using Vec2DComplex = std::vector<std::vector<std::complex<float>>>;
using Vec1D = std::vector<float>;
using Vec1DComplex = std::vector<std::complex<float>>;
using namespace juce;
#define mag2dB     juce::Decibels::gainToDecibels
#define dB2mag     juce::Decibels::decibelsToGain
#define j_Pi     juce::MathConstants<float>::pi
//==============================================================================
MainComponent::MainComponent()
    :forwardFFT1(fftOrder1), 
    inverseFFT1(fftOrder1), forwardFFT2(fftOrder2), inverseFFT2(fftOrder2), forwardFFT_TSM(fftOrderTSM), inverseFFT_TSM(fftOrderTSM),
    waveViewer(1), state(Stopped), 
    forwardFFT_visual_fft(fftOrder_visual_fft), 
    window_visual_fft(fftSize_visual_fft, juce::dsp::WindowingFunction<float>::hann),
    spectrogramTimeImage(juce::Image::ARGB, 200, 14, true),
    spectrogramImage(juce::Image::ARGB, 200, 8192, true),
    spectrogramImageDown(juce::Image::ARGB, 200, 8192, true),
    verticalMeterMaster([&]() { return rmsMaster;}),
    verticalMeterSine([&]() { return rmsSine; }),
    verticalMeterTrans([&]() { return rmsTrans; }),
    verticalMeterNoise([&]() { return rmsNoise; })
    //playButtonTest("C:/Users/Tantep/Desktop/testButton.png")
{
    

    addAndMakeVisible(waveViewer);
    waveViewer.setRepaintRate(100);
    
    waveViewer.setBufferSize(512);   
    waveViewer.setOpaque(false);
    waveViewer.setColours(scopeColor.withAlpha(0.0f), juce::Colours::goldenrod.withAlpha(0.9f));
    //waveViewer.setVisible(false);

    addAndMakeVisible(waveZoomTime);
    waveZoomTime.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    waveZoomTime.setTextBoxStyle(juce::Slider::NoTextBox,false,1,1);
    waveZoomTime.setRange(128,2048, 1);
    waveZoomTime.setValue(512);
    waveZoomTime.setSkewFactorFromMidPoint(512);
    waveZoomTime.setLookAndFeel(&otherLookAndFeelWaveZoom);
    waveZoomTime.setDoubleClickReturnValue(true, 512);
    waveZoomTime.onValueChange = [this]()
    {
        waveViewer.setBufferSize(waveZoomTime.getValue());
    };

    addAndMakeVisible(waveZoomAmp);
    waveZoomAmp.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    waveZoomAmp.setTextBoxStyle(juce::Slider::NoTextBox, false, 1, 1);
    waveZoomAmp.setRange(0.5,2,0.01);
    waveZoomAmp.setValue(1.f); 
    waveZoomAmp.setSkewFactorFromMidPoint(1);
    waveZoomAmp.setLookAndFeel(&otherLookAndFeelWaveZoom);
    waveZoomAmp.setDoubleClickReturnValue(true, 1);

    addAndMakeVisible(openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };
    openButton.setImages(true, true, true, openButtonNormal, 1.0f, {}, openButtonNormal, 0.7f, {}, openButtonDown, 1.0f, {});

    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this] { clearButtonClicked(); };
    clearButton.setImages(true, true, true, clearButtonNormal, 1.0f, {}, clearButtonNormal, 0.7f, {}, clearButtonDown, 1.0f, {});

    //addAndMakeVisible(saveButton);
    //saveButton.setButtonText("Save");
    //saveButton.onClick = [this] { saveButtonClicked(); };
    //saveButton.setImages(true, true, true, openClearButtonCleanBg, 1.0f, {}, openClearButtonCleanBg, 0.7f, {}, openClearButtonCleanBg, 1.0f, {});


    addAndMakeVisible(playButton);
    playButton.setButtonText("Play");  
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    playButton.setColour(juce::TextButton::textColourOffId, juce::Colours::ivory);
    playButton.setColour(juce::ComboBox::outlineColourId, juce::Colours::ivory);
    playButton.setEnabled(false);
    playButton.setImages(true, true, true, playButtonNormal, 1.0f, {}, playButtonNormal, 0.7f, {}, playButtonDown, 1.0f, {});
    //playButton.addListener(this);// Add a listener to the playButton
    

    addAndMakeVisible(backButton);
    backButton.setButtonText("<");
    backButton.onClick = [this] { backButtonClicked(); };
    backButton.setEnabled(false);
    backButton.setImages(true, true, true, backButtonNormal, 1.0f, {}, backButtonNormal, 0.7f, {}, backButtonDown, 1.0f, {});

    addAndMakeVisible(stopButton);
    stopButton.setButtonText("Stop");    
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);  
    stopButton.setEnabled(false);
    stopButton.setImages(true, true, true, stopButtonNormal, 1.0f, {}, stopButtonNormal, 0.7f, {}, stopButtonDown, 1.0f, {});



    addAndMakeVisible(verticalMeterSine);
    addAndMakeVisible(verticalMeterTrans);
    addAndMakeVisible(verticalMeterNoise);
    addAndMakeVisible(verticalMeterMaster);

    addAndMakeVisible(sineSlider);    
    sineSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    sineSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);     
    sineSlider.setRange(-72, 12, 0.5); sineSlider.setValue(0);  // Set the range and initial value of the slider
    sineSlider.setNumDecimalPlacesToDisplay(1); sineSlider.setTextValueSuffix(" dB");
    sineSlider.setDoubleClickReturnValue(true, 0);
    sineSlider.setSkewFactorFromMidPoint(-6.0f);
    sineSlider.setLookAndFeel(&otherLookAndFeelVertical);

    addAndMakeVisible(transientSlider);
    transientSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    transientSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    transientSlider.setRange(-72, 12, 0.5); transientSlider.setValue(0.f);
    transientSlider.setNumDecimalPlacesToDisplay(1); transientSlider.setTextValueSuffix(" dB");
    transientSlider.setDoubleClickReturnValue(true, 0);
    transientSlider.setSkewFactorFromMidPoint(-6.0f);
    transientSlider.setLookAndFeel(&otherLookAndFeelVertical);

    addAndMakeVisible(noiseSlider);
    noiseSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    noiseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    noiseSlider.setRange(-72, 12, 0.5); noiseSlider.setValue(0.f);
    noiseSlider.setNumDecimalPlacesToDisplay(1); noiseSlider.setTextValueSuffix(" dB");
    noiseSlider.setDoubleClickReturnValue(true, 0);
    noiseSlider.setSkewFactorFromMidPoint(-6.0f);
    noiseSlider.setLookAndFeel(&otherLookAndFeelVertical);

    addAndMakeVisible(masterSlider);
    masterSlider.setSliderStyle(juce::Slider::SliderStyle::LinearVertical);
    masterSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    masterSlider.setRange(-72, 12, 0.5); masterSlider.setValue(0.f); // Set the range and initial value of the slider
    masterSlider.setNumDecimalPlacesToDisplay(1); masterSlider.setTextValueSuffix(" dB");
    masterSlider.setDoubleClickReturnValue(true, 0);
    masterSlider.setSkewFactorFromMidPoint(-6.0f);
    masterSlider.setLookAndFeel(&otherLookAndFeelVertical);

    // Initialize button states
    sineButtonState = true;
    transientButtonState = true;
    noiseButtonState = true;
    masterButtonState = true;
    transientPreserveState = true;
    phaseLockState = true;

    addAndMakeVisible(sineButton);
    sineButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    sineButton.onClick = [this] { buttonClicked(&sineButton); };
    sineButton.setImages(true, true, true, sineButtonOn, 1.0f, {}, sineButtonOn, 0.7f, {}, sineButtonOn, 0.5f, {});

    addAndMakeVisible(transientButton);
    transientButton.setButtonText("Transient");
    transientButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    transientButton.onClick = [this] { buttonClicked(&transientButton); };
    transientButton.setImages(true, true, true, transButtonOn, 1.0f, {}, transButtonOn, 0.7f, {}, transButtonOn, 0.5f, {});

    addAndMakeVisible(noiseButton);
    noiseButton.setButtonText("Noise");
    noiseButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    noiseButton.onClick = [this] { buttonClicked(&noiseButton); };
    noiseButton.setImages(true, true, true, noiseButtonOn, 1.0f, {}, noiseButtonOn, 0.7f, {}, noiseButtonOn, 0.5f, {});

    addAndMakeVisible(masterButton);
    masterButton.setButtonText("Master");
    masterButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    masterButton.onClick = [this] { buttonClicked(&masterButton); };
    masterButton.setImages(true, true, true, masterButtonOn, 1.0f, {}, masterButtonOn, 0.7f, {}, masterButtonOn, 0.5f, {});



    addAndMakeVisible(transientPreserveButton);
    transientPreserveButton.setButtonText("TP");
    transientPreserveButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    transientPreserveButton.onClick = [this] { buttonClicked(&transientPreserveButton); };
    transientPreserveButton.setImages(true, true, true, transPreserveButtonActive, 1.0f, {}, transPreserveButtonActive, 0.7f, {}, transPreserveButtonDown, 1.0f, {});

    addAndMakeVisible(phaseLockButton);
    phaseLockButton.setButtonText("PLV");
    phaseLockButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    phaseLockButton.onClick = [this] { buttonClicked(&phaseLockButton); };
    phaseLockButton.setImages(true, true, true, pvButtonActive, 1.0f, {}, pvButtonActive, 0.7f, {}, pvButtonDown, 1.0f, {});


    addAndMakeVisible(matchTSMButton);
    matchTSMButton.setButtonText("Match");
    matchTSMButton.onClick = [this] { matchTSMButtonClicked(); };
    matchTSMButton.setImages(true, true, true, matchButtonNormal, 1.0f, {}, matchButtonNormal, 0.7f, {}, matchButtonDown, 1.0f, {});

    addAndMakeVisible(resetpitchTSMButton);
    resetpitchTSMButton.setButtonText("Reset");
    resetpitchTSMButton.onClick = [this] { resetpitchTSMButtonClicked();  };
    resetpitchTSMButton.setImages(true, true, true, resetButtonNormal, 1.0f, {}, resetButtonNormal, 0.7f, {}, resetButtonDown, 1.0f, {});


    // Create and initialize the label
    addAndMakeVisible(statusLabel);
    statusLabel.setText("No Audio File Loaded", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setFont(juce::Font(15.0f, juce::Font::plain));

    addAndMakeVisible(fileNameLable);
    fileNameLable.setText(" ", juce::dontSendNotification); // blank while there are no audio
    fileNameLable.setJustificationType(juce::Justification::centred);
    fileNameLable.setFont(juce::Font(14.0f, juce::Font::plain));


   
    addAndMakeVisible(tsmSlider);
    tsmSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    //tsmSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag);
    tsmSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    tsmSlider.setRange(0.1, 10, 0.01); tsmSlider.setValue(1.0);
    tsmSlider.setSkewFactorFromMidPoint(1.0);
    tsmSlider.setDoubleClickReturnValue(true, 1.0);
    tsmSlider.setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(tsmLabel);
    tsmLabel.setText("TSM Factor", juce::dontSendNotification);
    tsmLabel.setFont(juce::Font(14.0f, juce::Font::plain)); tsmLabel.setJustificationType(juce::Justification::centred);
    tsmLabel.attachToComponent(&tsmSlider, false); 

    addAndMakeVisible(resampleSlider);
    resampleSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    resampleSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 20);
    resampleSlider.setRange(0.25, 4, 0.01); resampleSlider.setValue(1.0);
    resampleSlider.setSkewFactorFromMidPoint(1.0);
    resampleSlider.setDoubleClickReturnValue(true, 1.0);
    resampleSlider.setNumDecimalPlacesToDisplay(2);
    addAndMakeVisible(resampleLabel);
    resampleLabel.setText("Playback Speed", juce::dontSendNotification);
    resampleLabel.setFont(juce::Font(14.0f, juce::Font::plain)); resampleLabel.setJustificationType(juce::Justification::centred);
    resampleLabel.attachToComponent(&resampleSlider, false); //


    addAndMakeVisible(progressBarSlider);
    progressBarSlider.setSliderStyle(juce::Slider::SliderStyle::LinearHorizontal);
    progressBarSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 100, 25);
    progressBarSlider.setRange(0,1); progressBarSlider.setValue(0);
    progressBarSlider.addListener(this);
    progressBarSlider.setLookAndFeel(&otherLookAndFeelProgressBar);

    addAndMakeVisible(hiResButton);
    hiResButton.setButtonText("Hi-Res");
    hiResButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen);
    hiResButton.onClick = [this] { buttonClicked(&hiResButton); };
    hiResButton.setImages(true, true, true, hiresButtonNormal, 1.0f, {}, hiresButtonNormal, 0.7f, {}, hiresButtonActive, 0.5f, {});


    formatManager.registerBasicFormats();

    setSize(600, 700);


    // Add the KeyListener to the component
    addKeyListener(this);
    setWantsKeyboardFocus(true);

    // for FFT display
    setOpaque(true);
    startTimerHz(2500);

    specgramSecCountReady = false;

    
}
void MainComponent::releaseResources()
{

    fileBuffer.setSize(0, 0);

}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
       // ----------------------------------------- WINDOW BACKGROUND start ----------------------------------------------
    g.fillAll(juce::Colours::transparentBlack);

    const auto bounds = getLocalBounds().toFloat();
    bgGradient = ColourGradient{ Colour::fromRGB(75,85,94).darker(0.25), bounds.getX(),bounds.getY(), Colour::fromRGB(49,52,59).darker(0.5), bounds.getBottom(),bounds.getRight(), true };
    bgGradient.addColour(0.3, Colour::fromRGB(112, 121, 131).darker(0.8));
    bgGradient.addColour(0.5, Colour::fromRGB(49, 52, 59).brighter(0.2));
    g.setGradientFill(bgGradient);
    g.fillRect(bounds);

    auto height = getHeight();
    auto width = getWidth();
    float cornerSize = 3.f;
    float rectLineThick = 2.f;
    g.setColour(Colours::lightgrey);
    g.setFont(11);
    g.drawText("Version 1.0 Build August 2023 By Tantep Sinjanakhom, Aalto Acoustics Lab", Rectangle<float>(12, height-13, 500, 10),Justification::topLeft);
    //==================== player component rect
    //float playerRectX = bounds.getX() + 10;
    //float playerRectY = bounds.getY() + 10;
    //float playerRectWidth = 180;
    //float playerRectHeight = 220;
    //playerRect = Rectangle<float>(playerRectX, playerRectY, playerRectWidth, playerRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(playerRect, cornerSize, rectLineThick);
    auto playerRectGradient = ColourGradient{ Colour::fromRGB(85,91,101).brighter(0.1), playerRect.getX(),playerRect.getY(), Colour::fromRGB(85,91,101).darker(0.1), playerRect.getBottom(),playerRect.getRight(), true };
    g.setGradientFill(playerRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(playerRect, cornerSize);


    //==================== mixer component rect
    //float mixerRectX = playerRect.getRight() + rectLineThick;
    //float mixerRectY = playerRect.getY();
    //float mixerRectWidth = 255;
    //float mixerRectHeight = playerRect.getHeight();
    //mixerRect = Rectangle<float>(mixerRectX, mixerRectY, mixerRectWidth, mixerRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(mixerRect, cornerSize, rectLineThick);
    auto mixerRectGradient = ColourGradient{ Colour::fromRGB(85,91,101), mixerRect.getX(),mixerRect.getY(), Colour::fromRGB(85,89,89).darker(0.1), mixerRect.getBottom(),mixerRect.getRight(), true };
    g.setGradientFill(mixerRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(mixerRect, cornerSize);

    //==================== tsm component rect
    //float tsmRectX = mixerRect.getRight() + rectLineThick;
    //float tsmRectY = mixerRect.getY();
    //float tsmRectWidth = 200;// width - tsmRectX - 10;
    //float tsmRectHeight = mixerRect.getHeight()/2;
    //Rectangle<float> tsmRect(tsmRectX, tsmRectY, tsmRectWidth, tsmRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(tsmRect, cornerSize, rectLineThick);
    //auto tsmRectGradient = ColourGradient{ Colour::fromRGB(89,89,89).brighter(0.1), tsmRect.getX(),tsmRect.getY(), Colour::fromRGB(85,91,101).darker(0.1), tsmRect.getBottom(),tsmRect.getRight(), true };
    auto tsmRectGradient = ColourGradient{ Colour::fromRGB(85,91,101).brighter(0.1), tsmRect.getX(),tsmRect.getY(), Colour::fromRGB(64,64,89).darker(2), tsmRect.getBottom(),tsmRect.getRight(), true };
    g.setGradientFill(tsmRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(tsmRect, cornerSize);

    //==================== resample component rect
    //float resampRectX = mixerRect.getRight() + rectLineThick;
    //float resampRectY = tsmRect.getBottom() ;
    //float resampRectWidth = 200;
    //float resampRectHeight = mixerRect.getHeight() / 2;
    //Rectangle<float> resampRect(resampRectX, resampRectY, resampRectWidth, resampRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(resampRect, cornerSize, rectLineThick);
    auto resampRectGradient = ColourGradient{ Colour::fromRGB(85,91,89).brighter(0.1), resampRect.getX(),resampRect.getY(), Colour::fromRGB(89,91,64).darker(2), resampRect.getBottom(),resampRect.getRight(), true };
    g.setGradientFill(resampRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(resampRect, cornerSize);

    //==================== waveform component rect
    //float waveformRectX = playerRect.getX();
    //float waveformRectY = playerRect.getBottom() + rectLineThick;
    //float waveformRectWidth = 310;
    //float waveformRectHeight = 215;
    //Rectangle<float> waveformRect(waveformRectX, waveformRectY, waveformRectWidth, waveformRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(waveformRect, cornerSize, rectLineThick);
    auto waveformRectGradient = ColourGradient{ Colour::fromRGB(85,91,101).brighter(0.2), waveformRect.getX(),waveformRect.getY(), Colour::fromRGB(85,91,89).darker(1), waveformRect.getBottom(),waveformRect.getRight(), true };
    g.setGradientFill(waveformRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(waveformRect, cornerSize);

    //==================== fft analyzer component rect
    //float fftAnaRectX = waveformRect.getX();
    //float fftAnaRectY = waveformRect.getBottom() + rectLineThick;
    //float fftAnaRectWidth = waveformRect.getWidth();
    //float fftAnaRectHeight = 215;
    //Rectangle<float> fftAnaRect(fftAnaRectX, fftAnaRectY, fftAnaRectWidth, fftAnaRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(fftAnaRect, cornerSize, rectLineThick);
    auto fftAnaRectGradient = ColourGradient{ Colour::fromRGB(89,89,90).darker(0.4),  fftAnaRect.getBottom(),fftAnaRect.getRight(), Colour::fromRGB(89,91,89).brighter(0.1),fftAnaRect.getX(),fftAnaRect.getY(), true };
    g.setGradientFill(fftAnaRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(fftAnaRect, cornerSize);

    //==================== specgram component rect
    //float specgramRectX = fftAnaRect.getRight() + rectLineThick;
    //float specgramRectY = mixerRect.getBottom() + rectLineThick;
    //float specgramRectWidth = resampRect.getRight() - specgramRectX;
    //float specgramRectHeight = fftAnaRect.getBottom() - waveformRect.getY();
    //Rectangle<float> specgramRect(specgramRectX, specgramRectY, specgramRectWidth, specgramRectHeight);
    g.setColour(Colours::black.brighter(0.1));
    g.drawRoundedRectangle(specgramRect, cornerSize, rectLineThick);
    auto specgramRectGradient = ColourGradient{ Colour::fromRGB(85,91,101).darker(0.1), specgramRect.getX(),specgramRect.getY(), Colour::fromRGB(89,89,90).darker(0.5), specgramRect.getBottom(),specgramRect.getRight(), true };
    g.setGradientFill(specgramRectGradient);
    g.setOpacity(0.95f);
    g.fillRoundedRectangle(specgramRect, cornerSize);

    // ----------------------------------------- WINDOW BACKGROUND end ----------------------------------------------

    int lineThick = 2;    
    auto defaultPlotLeft = 0.075f * width;
    auto defaultPlotRight = 0.375f * width;
    auto defaultPlotTop = 0.375f * height;
    // -----------------------------------------  WAVEFORM VIEWER start ----------------------------------------------

    int borderHeight = height / 4; // Calculate the height of each rectangle (height/4)    
    int borderWidth = width / 2 - 50; // Calculate the width of each border (width/2 - 50)   
    int verticalSpacing = 40; // Calculate the vertical spacing between the two borders

    float wfX = waveformRect.getX(), wfY = waveformRect.getY(), wfW = waveformRect.getWidth(), wfH = waveformRect.getHeight();

    juce::Rectangle<float> areaWaveViewerBorder(wfX + 40, wfY + 25, wfW-50, wfH-50);

    //waveViewer.setBounds(areaWaveViewerBorder.getX(), areaWaveViewerBorder.getY() + lineThick, wfW, wfH - 2 * lineThick);
    waveViewer.setBounds(wfX + 40, wfY + 25, wfW - 50, wfH - 50);


    waveZoomTime.setBounds(wfX + 40, wfY, wfW-50, 30);
    waveZoomAmp.setBounds(waveZoomTime.getX()-25, waveZoomTime.getY() + 20, 30, wfH-40);

    float rotationXAmp = areaWaveViewerBorder.getX()-35;
    float rotationYAmp = areaWaveViewerBorder.getY() + areaWaveViewerBorder.getHeight() / 2 + 35;
    AffineTransform transformAmp{};
    transformAmp = transformAmp.rotated(-0.5 * juce::MathConstants<float>::pi, rotationXAmp, rotationYAmp);

    g.setFont(juce::Font(15.0f, juce::Font::plain));
    g.addTransform(transformAmp);
    g.setColour(Colours::white);
    g.drawText("Amplitude", rotationXAmp, rotationYAmp, 100, 15, Justification::left, false);
    g.addTransform(transformAmp.inverted());
    g.drawText("Time", (areaWaveViewerBorder.getX() + areaWaveViewerBorder.getRight()) * 0.5-15, areaWaveViewerBorder.getBottom()+5, 50, 15, juce::Justification::left, false);
 
    juce::Colour fillColourWv1 = juce::Colour(0xff272740), fillColourWv2 = juce::Colour(0xff101016), fillColourWv3 = juce::Colour(0xff112126);

    ColourGradient gradientWv = juce::ColourGradient{ fillColourWv1, (float)areaWaveViewerBorder.getX(), (float)areaWaveViewerBorder.getY(),
        fillColourWv2, (float)areaWaveViewerBorder.getX() + (float)areaWaveViewerBorder.getWidth(), (float)areaWaveViewerBorder.getY() + (float)areaWaveViewerBorder.getHeight(), false };
    gradientWv.addColour(0.9, fillColourWv3);
    g.setGradientFill(gradientWv);
    g.fillRect(areaWaveViewerBorder);

    g.setColour(juce::Colours::grey);
    //vertical lines
    for (int i = 0; i < 10; ++i)
    {
        float x = areaWaveViewerBorder.getX() + i * (areaWaveViewerBorder.getWidth() / 10);
        auto localLineThick = (i == 5) ? 1.0f : 0.25f; // if i = 5 then thick =1 else thick = 0.25
        g.drawLine(x, areaWaveViewerBorder.getY(), x, areaWaveViewerBorder.getBottom(), localLineThick);

        if (i == 5) // Add micro ticks for the main vertical line
        {
            for (int j = 0; j < 50; ++j)
            {
                float microTickY = areaWaveViewerBorder.getY() + j * (areaWaveViewerBorder.getHeight() / 50);
                g.drawLine(x - 3, microTickY, x + 3, microTickY, 0.5f);
            }
        }
    }
    // horizontal lines
    for (int i = 0; i < 6; ++i)
    {
        float y = areaWaveViewerBorder.getY() + i * (areaWaveViewerBorder.getHeight() / 6);
        auto localLineThick = (i == 3) ? 1.0f : 0.25f;
        g.drawLine(areaWaveViewerBorder.getX(), y, areaWaveViewerBorder.getRight(), y, localLineThick);

        if (i == 3) // Add micro ticks for the main horizontal line
        {
            for (int j = 0; j < 50; ++j)
            {
                float microTickX = areaWaveViewerBorder.getX() + j * (areaWaveViewerBorder.getWidth() / 50);
                g.drawLine(microTickX, y - 3, microTickX, y + 3, 0.5f); // 
            }
        }
    }
    // draw border over everything

    g.setColour(juce::Colours::grey);
    //g.setGradientFill(gradientWv);
    g.drawRoundedRectangle(areaWaveViewerBorder, cornerSize, lineThick);


    // -----------------------------------------  WAVEFORM VIEWER end ----------------------------------------------

    // ----------------------------------------- FFT ANALYZER start ----------------------------------------------    


    // Define the second border (below) with adjusted y-coordinate to avoid overlap
    auto defaultPlotTopFFT = 0.675 * height;
    juce::Rectangle<float> areaFFTBorder(areaWaveViewerBorder.getX(), fftAnaRect.getY() + 10 , areaWaveViewerBorder.getWidth(), areaWaveViewerBorder.getHeight());



    //juce::Rectangle<float> areaFFTBackground(defaultPlotLeft + lineThick, defaultPlotTopFFT + lineThick,
    //    defaultPlotRight - 2 * lineThick, borderHeight - 2 * lineThick);

    auto areaFFTBackground = areaFFTBorder;


    juce::Colour fillColourFFT1 = juce::Colour(0xff272740), fillColourFFT2 = juce::Colour::fromRGB(5, 8, 10), fillColourFFT3 = juce::Colour::fromRGB(9, 28, 36);

    ColourGradient gradientFFT = juce::ColourGradient{ fillColourWv1, areaFFTBackground.getX(), areaFFTBackground.getY() ,
        fillColourFFT2.darker(1), areaFFTBackground.getRight() , areaFFTBackground.getHeight(), true };
    gradientFFT.addColour(0.8, fillColourFFT3); gradientFFT.addColour(0.95, juce::Colour::fromRGB(13, 18, 16));
    g.setGradientFill(gradientFFT);
    g.fillRoundedRectangle(areaFFTBackground, cornerSize);


    if (fftFs != float(systemFs) && float(systemFs) != 0.f)
    {
        fftFs = float(systemFs);
        repaint();
    }
    juce::Array<float> freqs
    {
        20, 30, 40, 50, 100,
        200, 300, 400, 500, 1000,
        2000, 3000, 4000, 5000, 10000, 20000,fftFs / 2
    };


    g.setColour(juce::Colours::white);
    for (auto f : freqs)
    {
        auto normX = juce::mapFromLog10(f, 10.f, fftFs / 2);
        normX = juce::jlimit(0.f, 1.f, normX);  // Clamp normX to [0, 1]
        auto x = areaFFTBackground.getX() + normX * areaFFTBackground.getWidth();
        auto localLineThick = (f == 100 || f == 1000 || f == 10000) ? 1.0f : 0.25f;
        g.drawLine(x, areaFFTBackground.getY(), x, areaFFTBackground.getBottom(), localLineThick); // by using drawLine we can control the thickness

        //auto freqLabel = int(f);
        juce::String freqLabelStr;// = juce::String(f) + "Hz";
        std::set<float> freqLabelsToIgnore = { 30, 40, 200, 400, 2000, 4000 , fftFs / 2 };
        if (freqLabelsToIgnore.find(f) != freqLabelsToIgnore.end())
        {
            freqLabelStr = " ";
        }
        else if (int(f) >= int(1000))
        {
            freqLabelStr = juce::String(f / 1000) + "k";
        }
        else
        {
            freqLabelStr = juce::String(f);
        }

        // Tilt the text by applying an AffineTransform
        juce::AffineTransform transform;
        float rotationAngleDegrees = 45.0f; // Adjust the angle as per your preference
        float rotationAngleRadians = j_Pi * rotationAngleDegrees / 180.0f;
        transform = transform.rotated(rotationAngleRadians, x, areaFFTBackground.getBottom()+3); // Use the starting coordinates as the pivot point
        g.addTransform(transform);
        g.setFont(juce::Font(12.f, juce::Font::plain));
        g.drawText(freqLabelStr, x + 2, areaFFTBackground.getBottom(), 35, 10, juce::Justification::left, false);

        // Reset the AffineTransform
        g.addTransform(transform.inverted());
    }

    juce::Array<float> gain{ -72,-60, -48, -36, -24,-12, -6, 0};


    for (auto gdB : gain)
    {
        auto y = juce::jmap(gdB, -72.f, 0.f, float(areaFFTBackground.getBottom()), float(areaFFTBackground.getY()));
        //g.drawHorizontalLine(int(y), areaFFTBackground.getX(), areaFFTBackground.getRight());
        g.drawLine(areaFFTBackground.getX(), y, areaFFTBackground.getRight(), y, 0.25f); //
        g.setFont(juce::Font(12.f, juce::Font::plain));
        g.drawText(juce::String(gdB), areaFFTBackground.getX() - 22, int(y) - 8, 20, 10, juce::Justification::right, false);
    }

    //  // Draw "Magnitude (dB)"  with 90-degree rotation
    juce::AffineTransform transform;

    float rotationX = rotationXAmp-2;
    float rotationY = (areaFFTBackground.getY() + areaFFTBackground.getBottom()) * 0.54f; // Center Y position

    transform = transform.rotated(-0.5 * juce::MathConstants<float>::pi, rotationX, rotationY);

    g.setFont(juce::Font(14.0f, juce::Font::plain));
    g.addTransform(transform);
    g.drawText("Magnitude [dB]", rotationX, rotationY, 100, 15, juce::Justification::left, false);
    g.addTransform(transform.inverted());
    g.drawText("Frequency [Hz]", areaFFTBackground.getX() + areaFFTBackground.getWidth()/2 - 40, areaFFTBackground.getBottom() + 20, 100, 15, juce::Justification::centredTop, false);


    // fft itself

    g.setColour(juce::Colours::goldenrod);
    //drawFrame(g, defaultPlotLeft + lineThick, areaWaveViewerBorder.getBottom() + verticalSpacing + lineThick, defaultPlotRight - 2 * lineThick, borderHeight - 2 * lineThick);
    //drawFrame(g, defaultPlotLeft + lineThick, defaultPlotTopFFT + lineThick*2, defaultPlotRight - 2 * lineThick, borderHeight - 2 * lineThick);
    //drawFrame(g, areaFFTBorder.getX(), areaFFTBorder.getY() + lineThick * 2, areaFFTBorder.getWidth(), areaFFTBorder.getHeight() -  lineThick*2);
    drawFrame(g, areaFFTBackground.getX(), areaFFTBackground.getY(), areaFFTBackground.getWidth(), areaFFTBackground.getHeight());
    g.setColour(juce::Colours::grey);
    //g.setGradientFill(gradientWv);
    //g.drawRect(areaFFTBorder, lineThick);
    g.drawRoundedRectangle(areaFFTBorder, cornerSize, lineThick*1.1);

    //// -----------------------------------------  FFT ANALYZER end ----------------------------------------------



    // -----------------------------------------  SPECTROGRAM start ----------------------------------------------
    //g.setColour(juce::Colours::grey);
    //g.fillRect(40, 500, 600, 150);
    //g.setOpacity(1.0f);
    //g.drawImageWithin(spectrogramImage, 40, 500, 600, 150, juce::RectanglePlacement::stretchToFit);
    g.setImageResamplingQuality(juce::Graphics::ResamplingQuality::highResamplingQuality);
    auto defaultPlotTopSpec = defaultPlotTop;
    auto defaultPlotLeftSpec = 0.5625 * width;
    auto defaultPlotRightSpec = 0.4 * width;
    //juce::Rectangle<float> areaSpectrogram(width / 2 + 50, 300, width / 2 - 80, areaFFTBackground.getBottom() - 300);

    float spX = specgramRect.getX(), spY = specgramRect.getY(), spW = specgramRect.getWidth(), spH = specgramRect.getHeight();

    juce::Rectangle<float> areaSpectrogram(spX + 45, spY + 15, spW - 50, spH - 55);

    juce::Rectangle<float> areaSpectrogramTime(areaSpectrogram.getX(), areaSpectrogram.getBottom(), areaSpectrogram.getWidth(), 20);
    
    spectrogramImage.rescaled(areaSpectrogram.getWidth(), areaSpectrogram.getHeight());

    //spectrogramTimeImage.rescaled(areaSpectrogramTime.getWidth(), 20);

    // Adjust the y-coordinate for the second spectrogram plot
    float yNyqDown = juce::jmap(juce::mapFromLog10(fftFs/8, 10.f, fftFs / 2), 0.f, 1.f, float(areaSpectrogram.getBottom()), float(areaSpectrogram.getY()));

    juce::Rectangle<float> areaSpectrogramDown(areaSpectrogram.getX(), yNyqDown, areaSpectrogram.getWidth(), areaSpectrogram.getBottom() - yNyqDown);
    spectrogramImageDown.rescaled(areaSpectrogramDown.getWidth(), areaSpectrogramDown.getHeight());

    

    
    ColourGradient gradientSpec = juce::ColourGradient{ fillColourFFT1.darker(1), areaSpectrogram.getX(),(float)areaSpectrogram.getY() ,
        fillColourFFT2, areaSpectrogram.getRight(), areaSpectrogram.getBottom(), true };
    gradientSpec.addColour(0.8, fillColourFFT3);
    g.setGradientFill(gradientSpec);
    g.fillRoundedRectangle(areaSpectrogram.expanded(0.2), cornerSize);
    //g.setColour(juce::Colours::grey);
    //g.fillRoundedRectangle(areaSpectrogramDown, cornerSize);
    
    g.setColour(juce::Colours::grey);
    if (!hiResState)
    {
        //g.setOpacity(0.1);
        g.drawImage(spectrogramImage, areaSpectrogram);
        //g.setOpacity(1.0);
        g.drawRoundedRectangle(areaSpectrogram.getX() - lineThick, areaSpectrogram.getY() - lineThick, areaSpectrogram.getWidth() + 2 * lineThick, areaSpectrogram.getHeight() + lineThick, cornerSize, lineThick);

    }
    else
    {
        g.drawImage(spectrogramImageDown, areaSpectrogramDown);
        g.drawRoundedRectangle(areaSpectrogramDown.getX() - lineThick, areaSpectrogramDown.getY() - lineThick, areaSpectrogramDown.getWidth() + 2 * lineThick, areaSpectrogramDown.getHeight() + lineThick, cornerSize, lineThick);

    }
   
    
    g.drawImage(spectrogramTimeImage, areaSpectrogramTime);

    
    for (auto f : freqs)
    {
        // Map 'f' logarithmically between 20.f and fftFs / 2
        auto mappedValue = juce::mapFromLog10(f, 10.f, fftFs / 2);

        // Map 'mappedValue' linearly to the y-coordinate within the areaSpectrogram
        auto y = juce::jmap(mappedValue, 0.f, 1.f, float(areaSpectrogram.getBottom()), float(areaSpectrogram.getY()));

        juce::String freqLabelStr;// = juce::String(f) + "Hz";
        std::set<float> freqLabelsToIgnore = { 30, 40, 200, 400, 2000, 4000 , fftFs / 2 };
        if (freqLabelsToIgnore.find(f) != freqLabelsToIgnore.end())
        {
            freqLabelStr = " ";
        }
        else if (int(f) >= int(1000))
        {
            freqLabelStr = juce::String(f / 1000) + "k";
        }
        else
        {
            freqLabelStr = juce::String(f);
        }
        g.setColour(juce::Colours::dimgrey);
        g.drawLine(areaSpectrogram.getX(), y, areaSpectrogram.getRight(), y, 0.25f);
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::Font(13.0f, juce::Font::plain));
        g.drawText(freqLabelStr, areaSpectrogram.getX() - 35, int(y) - 8, 30, 16, juce::Justification::right, false);
    }
    
    float rotationXSpec = areaSpectrogram.getX() - 43;
    float rotationYSpec = (areaSpectrogram.getY() + areaSpectrogram.getBottom()) * 0.55;
    AffineTransform transformSpec{};
    transformSpec = transformSpec.rotated(-0.5 * juce::MathConstants<float>::pi, rotationXSpec, rotationYSpec);

    g.setFont(juce::Font(14.0f, juce::Font::plain));
    g.addTransform(transformSpec);
    g.setColour(Colours::lightgrey);
    g.drawText("Frequency [Hz]", rotationXSpec, rotationYSpec, 100, 15, juce::Justification::left, false);
    g.addTransform(transformSpec.inverted());
    auto timeString = String("Time - 1 sec interval : |-|");
    float timeStringWidth = g.getCurrentFont().getStringWidth(timeString);
    float timeStringX = (areaSpectrogram.getX() + areaSpectrogram.getWidth() / 2) - (timeStringWidth / 2);

    g.drawText(timeString, timeStringX, areaSpectrogram.getBottom() + 20, timeStringWidth, 15, juce::Justification::left, false);


    for (auto x = 0; x < spectrogramTimeImage.getWidth(); ++x)
    {
        spectrogramTimeImage.setPixelAt(x, spectrogramTimeImage.getHeight() / 2, juce::Colours::silver);
        //specgramSecCountReady = false;
    }

    //auto specTimeRect = juce::Rectangle<float> (areaSpectrogram.getX(), areaSpectrogram.getBottom(), areaSpectrogram.getWidth(), spectrogramTimeImage.getHeight()/2);
    //g.drawRoundedRectangle(specTimeRect, cornerSize/2, lineThick/2);
    hiResButton.setBounds(specgramRect.getRight() - 43, specgramRect.getBottom()-25, 40, 25);



    // -----------------------------------------  SPECTROGRAM end ----------------------------------------------


    juce::Rectangle<float> areaLogoBackground(playerRect.getX() + 5, playerRect.getY() + 5, playerRect.getWidth()-10, 60);
    g.setColour(juce::Colours::goldenrod);
    g.fillRoundedRectangle(areaLogoBackground,3.0f);
    iconLemon = juce::ImageCache::getFromMemory(BinaryData::SiTraNoPluslogo_png, BinaryData::SiTraNoPluslogo_pngSize);
    float logoSize = 40;    
    g.drawImageWithin(iconLemon, areaLogoBackground.getRight() - logoSize, areaLogoBackground.getY() + logoSize/2, logoSize, logoSize, juce::RectanglePlacement::stretchToFit);

    g.setColour(juce::Colours::black);
    float fontSize = 34;
    juce::Font font(fontSize);
    font.setBold(true);
    g.setFont(font);
 /*   g.drawFittedText("SiTraNo", playerRect.getX() + 10 , playerRect.getY() + 10, areaLogoBackground.getWidth() - logoSize, 56, juce::Justification::left, true);
    g.drawFittedText("+", playerRect.getX() + 10, playerRect.getY() + 10, areaLogoBackground.getWidth() - logoSize, 56, juce::Justification::left, true);*/

    String sitranoText = "SiTraNo";
    float sitranoTextWidth = font.getStringWidth(sitranoText);
    float sitranoTextHeight = font.getHeight();
    float sitranoTextX = playerRect.getX() + 10;
    float sitranoTextY = playerRect.getY() + 6;

    g.drawFittedText(sitranoText, sitranoTextX, sitranoTextY, areaLogoBackground.getWidth() - logoSize, 56, juce::Justification::left, true);

    String plusText = "+";
    float plusTextWidth = font.getStringWidth(plusText);

    // Position the plusText on the top right corner of the text
    float plusTextX = sitranoTextX + sitranoTextWidth - plusTextWidth/4;
    float plusTextY = sitranoTextY;// -sitranoTextHeight + (sitranoTextHeight - font.getHeight()) * 0.5f;

    g.drawFittedText(plusText, plusTextX, plusTextY, plusTextWidth, sitranoTextHeight, juce::Justification::left, true);


    g.setOpacity(0.7f);
    g.drawImage(PlayerButtonCleanBg, backButton.getBounds().toFloat(), juce::RectanglePlacement::centred);
    g.drawImage(PlayerButtonCleanBg, playButton.getBounds().toFloat(), juce::RectanglePlacement::centred);
    g.drawImage(PlayerButtonCleanBg, stopButton.getBounds().toFloat(), juce::RectanglePlacement::centred);
    g.setOpacity(1.0f);  
}

void MainComponent::resized()
{
  
    auto height = getHeight();
    auto width = getWidth();

    

    //0.05 * getWidth() = 5 % of 600 = 30
    auto defaultLeft = 0.05 * width; // 30

    
    openButton.setBounds(30,       100, 70, 30);
    clearButton.setBounds(30 + 70, 100, 70, 30);
    //saveButton.setBounds(clearButton.getRight() + 5, clearButton.getY()-5, 50, 50);

    backButton.setBounds(getX() + 10.5f, openButton.getBottom()+3.f, 60, 30);
    playButton.setBounds(backButton.getRight() + 1.f, openButton.getBottom() + 3.f, 60, 30);
    stopButton.setBounds(playButton.getRight() + 0.5f, openButton.getBottom() + 3.f, 60, 30);

    statusLabel.setBounds(backButton.getX()-10, openButton.getY()-22, 200, 20);
    fileNameLable.setBounds(backButton.getX(), playButton.getBottom()+5, 180, 20);

    progressBarSlider.setBounds(backButton.getX(), fileNameLable.getBottom()-2, 180, 50);

   
    sineButton.setBounds(stopButton.getRight()       + 2.f, getY() + 15.f , 60, 25);
    transientButton.setBounds(sineButton.getRight()  + 3.f, getY() + 15.f , 60, 25);
    noiseButton.setBounds(transientButton.getRight() + 3.f, getY() + 15.f , 60, 25);
    masterButton.setBounds(noiseButton.getRight()    + 3.f, getY() + 15.f , 60, 25);

    sineSlider.setBounds(sineButton.getX()-10, sineButton.getBottom()-3, 50, 190);
    verticalMeterSine.setBounds(sineSlider.getX() + 37, sineButton.getBottom(), 40, 155);

    transientSlider.setBounds(transientButton.getX() - 10, transientButton.getBottom() - 3, 50, 190);
    verticalMeterTrans.setBounds(transientSlider.getX() + 37, transientButton.getBottom(), 40, 155);

    noiseSlider.setBounds(noiseButton.getX() - 10, noiseButton.getBottom() - 3, 50, 190);
    verticalMeterNoise.setBounds(noiseSlider.getX() + 37, noiseButton.getBottom(), 40, 155);

    // master level component slider
    masterSlider.setBounds(masterButton.getX() - 10, masterButton.getBottom() - 3, 50, 190);
    verticalMeterMaster.setBounds(masterSlider.getX()+37, masterButton.getBottom()  , 40, 155);



    // TSM and Resample component
    //tsmLabel.setBounds(0.725 * width, 20, 80, 30);
    tsmSlider.setBounds(masterButton.getX()+62, masterButton.getY()+20, 210, 55);
    tsmSlider.setLookAndFeel(&otherLookAndFeelHorizontal);

    phaseLockButton.setBounds(tsmSlider.getX() + 20, tsmSlider.getBottom() - 10, 75, 50);
    transientPreserveButton.setBounds(phaseLockButton.getRight()+20, tsmSlider.getBottom() - 10, 75, 50);

    //resampleLabel.setBounds(0.725 * width, 120, 80, 30);
    resampleSlider.setBounds(masterButton.getX() + 62, transientPreserveButton.getBottom() + 15, 210, 55);
    resampleSlider.setLookAndFeel(&otherLookAndFeelHorizontal);

    matchTSMButton.setBounds(resampleSlider.getX() + 20, resampleSlider.getBottom() - 10, 75, 50);
    resetpitchTSMButton.setBounds(matchTSMButton.getRight() + 20, resampleSlider.getBottom() - 10, 75, 50);
    

    hiResButton.setBounds(resetpitchTSMButton.getRight()-50, resetpitchTSMButton.getBottom() + 370,40,40);

    //resampleButton.setBounds(650, 50, 100, 30);


    //horizontalMeterMaster.setBounds(550, 100, 180, 20);
    

    int lineThick = 2;
    
    int borderHeight = height / 4;
    int borderWidth = width / 2 - 50;
    auto defaultPlotLeft = 0.075f * width;
    auto defaultPlotRight = 0.375f * width;
    auto defaultPlotTop = 0.375f * height;

    /*waveViewer.setBounds(defaultPlotLeft, defaultPlotTop + lineThick, defaultPlotRight - 2 * lineThick, borderHeight - 2 * lineThick);*/

    //waveZoomTime.setBounds(backButton.getX() + 40, progressBarSlider.getBottom() - 5, borderWidth-40, 30);
    //waveZoomAmp.setBounds(waveZoomTime.getRight(), waveZoomTime.getY()+15, 30, borderHeight+20);

    float rectLineThick = 2.f;

    float playerRectX = 10;
    float playerRectY = 10;
    float playerRectWidth = 180;
    float playerRectHeight = 220;
    playerRect = Rectangle<float>(playerRectX, playerRectY, playerRectWidth, playerRectHeight);

    float mixerRectX = playerRect.getRight() + rectLineThick;
    float mixerRectY = playerRect.getY();
    float mixerRectWidth = 255;
    float mixerRectHeight = playerRect.getHeight();
    mixerRect = Rectangle<float>(mixerRectX, mixerRectY, mixerRectWidth, mixerRectHeight);

    float tsmRectX = mixerRect.getRight() + rectLineThick;
    float tsmRectY = mixerRect.getY();
    float tsmRectWidth = 200;// width - tsmRectX - 10;
    float tsmRectHeight = mixerRect.getHeight() / 2;
    tsmRect = Rectangle<float>(tsmRectX, tsmRectY, tsmRectWidth, tsmRectHeight);

    float resampRectX = mixerRect.getRight() + rectLineThick;
    float resampRectY = tsmRect.getBottom();
    float resampRectWidth = 200;
    float resampRectHeight = mixerRect.getHeight() / 2;
    resampRect = Rectangle<float>(resampRectX, resampRectY, resampRectWidth, resampRectHeight);

    float wave_fft_height = (getHeight() - playerRect.getHeight()) / 2 - 10 - 2*lineThick;

    float waveformRectX = playerRect.getX();
    float waveformRectY = playerRect.getBottom() + rectLineThick;
    float waveformRectWidth = getWidth() / 2 -10;
    float waveformRectHeight = wave_fft_height;
    waveformRect = Rectangle<float>(waveformRectX, waveformRectY, waveformRectWidth, waveformRectHeight);

    float fftAnaRectX = waveformRect.getX();
    float fftAnaRectY = waveformRect.getBottom() + rectLineThick;
    float fftAnaRectWidth = waveformRect.getWidth();
    float fftAnaRectHeight = wave_fft_height;
    fftAnaRect = Rectangle<float>(fftAnaRectX, fftAnaRectY, fftAnaRectWidth, fftAnaRectHeight);

    float specgramRectX = fftAnaRect.getRight() + rectLineThick;
    float specgramRectY = mixerRect.getBottom() + rectLineThick;
    float specgramRectWidth = waveformRect.getWidth();
    float specgramRectHeight = fftAnaRect.getBottom() - waveformRect.getY();
    specgramRect = Rectangle<float>(specgramRectX, specgramRectY, specgramRectWidth, specgramRectHeight);
}



MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();

    
    waveZoomTime.setLookAndFeel(nullptr);
    waveZoomAmp.setLookAndFeel(nullptr);
    sineSlider.setLookAndFeel(nullptr);
    transientSlider.setLookAndFeel(nullptr);
    noiseSlider.setLookAndFeel(nullptr);
    masterSlider.setLookAndFeel(nullptr);
    tsmSlider.setLookAndFeel(nullptr);
    resampleSlider.setLookAndFeel(nullptr);
    progressBarSlider.setLookAndFeel(nullptr);
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    auto circBufferSize = fftSizeTSM * 10;

    //outputCircularBuffer_S.clear();
    //outputCircularBuffer_T.clear();
    //outputCircularBuffer_N.clear();
    
    outputCircularBuffer_S.clear();
    outputCircularBuffer_T.clear();
    outputCircularBuffer_N.clear();

    outputCircularBuffer_S.setSize(2, (int)circBufferSize);
    outputCircularBuffer_T.setSize(2, (int)circBufferSize);
    outputCircularBuffer_N.setSize(2, (int)circBufferSize);

    waveViewer.clear();

    phi0_S.resize(fftSizeTSM); phi0_T.resize(fftSizeTSM); phi0_N.resize(fftSizeTSM);
    psi_S.resize(fftSizeTSM); psi_T.resize(fftSizeTSM); psi_N.resize(fftSizeTSM);

    decay_bins.resize(fftSizeTSM, false);
    decay_gains.resize(fftSizeTSM, 1.0f);
    attack_bins.resize(fftSizeTSM, false);
    attack_gains.resize(fftSizeTSM, 1.0f);
    bins_to_reset.resize(fftSizeTSM, false);
    int decay_length = 0;
    int reset_frame = 0;



   Rt_0 = 0.f, Rt_d1 = 0.f, Rt_d2 = 0.f;

    sm_masterL = 0.0f;
    sm_SineL = 0.0f;
    sm_NoiseL = 0.0f;
    sm_TransientL = 0.0f;

    rmsMaster = -100.f;
    rmsSine = -100.f;
    rmsTrans = -100.f;
    rmsNoise = -100.f;

    instantFrameRatio = 0.f;
    instantFrameRatio_prev = 0.f;

    // Initialize the filter object
    lowpassFilterCoeff = makeLowCutFilter(sampleRate);

    // Initialize the process spec
    processSpec.sampleRate = sampleRate;
    processSpec.maximumBlockSize = samplesPerBlockExpected; // Set an appropriate block size
    processSpec.numChannels = 1; // Set the number of channels (stereo in this case)

    // Prepare the filter with the process spec and coefficients
    lowpassFilter.prepare(processSpec);
    lowpassFilter.coefficients = *lowpassFilterCoeff.getFirst();

   //    for (int i = 0; i < lowpassFilterCoeff.size(); ++i)
   //{
   //    const juce::dsp::IIR::Coefficients<float>& coefficients = *lowpassFilterCoeff[i];

   //    const float* rawCoefficients = coefficients.getRawCoefficients();

   //    DBG("Raw coefficients for filter " << i + 1 << ":");
   //    for (int j = 0; j < (sizeof(rawCoefficients)/sizeof(float)); ++j)
   //    {
   //        DBG("  Coefficient " << j << ": " << rawCoefficients[j]);
   //    }
   //}


   //for (int i = 0; i < lowpassFilterCoeff.size(); ++i)
   //{
   //    const juce::dsp::IIR::Coefficients<float>& coefficients = *lowpassFilterCoeff[i];

   //    DBG("filterOrder" << coefficients.getFilterOrder());
   //    auto a = coefficients.getRawCoefficients();
   //    DBG("size:" << sizeof(a) / sizeof(float));
   //    for( int j = 0; j < (sizeof(a) / sizeof(float));++j)
   //        DBG("Coefficients: " << j << " " << a[j]);
   //}

}

float calculateRMS(const float* data, int numSamples)
{
    float sumOfSquares = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = data[i];
        sumOfSquares += sample * sample;
    }

    float meanSquared = sumOfSquares / static_cast<float>(numSamples);
    float rms = std::sqrt(meanSquared);

    return rms;
}

float calculateRMSVec(const Vec1D data, int numSamples)
{
    float sumOfSquares = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float sample = data[i];
        sumOfSquares += sample * sample;
    }

    float meanSquared = sumOfSquares / static_cast<float>(numSamples);
    float rms = std::sqrt(meanSquared);

    return rms;
}



void MainComponent::updateAntialiasFilter()
{
    lowpassFilterCoeff = makeLowCutFilter(systemFs);
}
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{

    auto numOutputChannels = bufferToFill.buffer->getNumChannels();

    auto outputSamplesRemaining = bufferToFill.numSamples;            // Some system bufferSize e.g. 441 samples
    auto outputSamplesOffset = bufferToFill.startSample;              // Starts at 0

    auto circBufferSize = outputCircularBuffer_S.getNumSamples();

    float alpha = roundToDecimal((float)tsmSlider.getValue(), 2);
    int nHopS = nWin_syn / 8;
    int nHopA = std::floor(nHopS / alpha);
    float resampleRate = (float)resampleSlider.getValue();
    float ola_coef_syn = ola_norm_coef(win_hann_syn, nHopS);


    while (outputSamplesRemaining > 0)
    {
        auto bufferSamplesRemaining = outputCircularBuffer_S.getNumSamples() - position;

        auto samplesThisTime = juce::jmin(outputSamplesRemaining, bufferSamplesRemaining); 

        auto channel = 0; // processing just the left ch

        auto* destBuffer = bufferToFill.buffer->getWritePointer(channel, outputSamplesOffset);

        //DBG("sineBuff.resize()");
        sineBuff.resize(samplesThisTime);
        transBuff.resize(samplesThisTime);
        NoiseBuff.resize(samplesThisTime);

        nFramesNew = nFrames_prev; // init nFrames
        if (frameIndex >= nFramesNew)
        {
            frameIndex = 0;
        }

        for (auto sample = 0; sample < samplesThisTime; ++sample)
        {
            sampleCount++;
            if (sampleCount == systemFs)
            {
                sampleCount = 0;
                specgramSecCountReady = true;
            }

            tar_masterL = masterButtonState * juce::Decibels::decibelsToGain((float)masterSlider.getValue(), -72.0f);
            tar_SineL = sineButtonState * juce::Decibels::decibelsToGain((float)sineSlider.getValue(), -72.0f);
            tar_TransientL = transientButtonState * juce::Decibels::decibelsToGain((float)transientSlider.getValue(), -72.0f);
            tar_NoiseL = noiseButtonState * juce::Decibels::decibelsToGain((float)noiseSlider.getValue(), -72.0f);

            sm_masterL = sm_masterL - (1.f - tau) * (sm_masterL - tar_masterL);
            sm_SineL = sm_SineL - (1.f - tau) * (sm_SineL - tar_SineL);
            sm_TransientL = sm_TransientL - (1.f - tau) * (sm_TransientL - tar_TransientL);
            sm_NoiseL = sm_NoiseL - (1.f - tau) * (sm_NoiseL - tar_NoiseL);

           // DBG("sineBuff[sample] = outputCircularBuffer_S.getSample(channel, outReadPtr) * sm_SineL;");
            sineBuff[sample] = outputCircularBuffer_S.getSample(channel, outReadPtr) * sm_SineL;
            transBuff[sample] = outputCircularBuffer_T.getSample(channel, outReadPtr) * sm_TransientL;
            NoiseBuff[sample] = outputCircularBuffer_N.getSample(channel, outReadPtr) * sm_NoiseL;

            destBuffer[sample] = sm_masterL * (sineBuff[sample] + transBuff[sample] + NoiseBuff[sample]);

            //DBG("outputCircularBuffer_S.setSample(channel, (outReadPtr) % circBufferSize, 0.0f);");
            outputCircularBuffer_S.setSample(channel, (outReadPtr) % circBufferSize, 0.0f);
            outputCircularBuffer_T.setSample(channel, (outReadPtr) % circBufferSize, 0.0f);
            outputCircularBuffer_N.setSample(channel, (outReadPtr) % circBufferSize, 0.0f);
            outReadPtr++;
            if (outReadPtr >= circBufferSize)
            {
                outReadPtr = 0;
            }


            if (++hopCount >= nHopS)
                //DBG("if (++hopCount >= nHopS)");
            {
                
                if ((nHopA_prev != nHopA) || (resampleRate_prev != resampleRate))
                {
                    //DBG("if ((nHopA_prev != nHopA) || (resampleRate_prev != resampleRate))");
                    if (resampleRate_prev != resampleRate)
                    {
                        //DBG("if (resampleRate_prev != resampleRate)");
                        int kDownfactor = 100 * resampleRate;
                        int kUpfactor = 100;
                        resample<float>(kUpfactor, kDownfactor, yS_og, yS);
                        resample<float>(kUpfactor, kDownfactor, yT_og, yT);
                        resample<float>(kUpfactor, kDownfactor, yN_og, yN);
                        resampleRate_prev = resampleRate;

                    }
                    //DBG("decSigLen = yS.size(); yS.shrink_to_fit(); ");
                    decSigLen = yS.size();
                    //yS.shrink_to_fit();
                    //yT.shrink_to_fit();
                    //yN.shrink_to_fit();

                    int nRows_TSM = fftSizeTSM; 
                    int nCols_TSM = std::floor((decSigLen_og - fftSizeTSM) / std::round(resampleRate * nHopS / alpha) + 1); // nFrames

                    nFramesNew = nCols_TSM;
                    float framePosRatio = (float)frameIndex / (float)nFrames_prev;

                    /////============================== transient reposition process ==============================///
                    int nFrames_TSM = nCols_TSM;
                    DBG("Start transientReposition");
                    transientReposition(yT_tsm, yT_og, peakLocs, alpha / resampleRate, nFrames_TSM, nHopS, nWin_syn);
                    DBG("DONE transientReposition");
                    //yT_tsm.shrink_to_fit();
                    // reassignment
                    nHopA_prev = nHopA;
                    nFrames_prev = nFramesNew;

                    frameIndex = std::floor(framePosRatio * nFramesNew);

                    if (frameIndex >= nFramesNew)
                    {
                        frameIndex = 0;
                    }
                    ola_coef_syn = ola_norm_coef(win_hann_syn, nHopA);

                }

                hopCount = 0;

                Vec1DComplex XS_current(fftSizeTSM);
                Vec1DComplex XT_current(fftSizeTSM);
                Vec1DComplex XN_current(fftSizeTSM);
                if (frameIndex >= nFramesNew)
                {
                    frameIndex = 0;
                }

                Vec1DComplex tempGrainS = getFrameFromInputSignal(frameIndex, fftSizeTSM, nHopA, win_hann_syn, yS);
                Vec1DComplex tempGrainT = getFrameFromInputSignal(frameIndex, fftSizeTSM, nHopA, win_hann_syn, yT);

                Vec1D tempGrainT_Real = getFrameFromInputSignal_Transient(frameIndex, fftSizeTSM, nHopS, win_hann_syn, yT_tsm);

                Vec1DComplex tempGrainN = getFrameFromInputSignal(frameIndex, fftSizeTSM, nHopA, win_hann_syn, yN);
                //Vec1D RnFrame = getFrameFromInputSignal_RtRn(frameIndex, fftSizeTSM, nHopA, win_hann_syn, RnVec);

                forwardFFT_TSM.perform(tempGrainS.data(), XS_current.data(), false);
                forwardFFT_TSM.perform(tempGrainT.data(), XT_current.data(), false);
                forwardFFT_TSM.perform(tempGrainN.data(), XN_current.data(), false);

                if (alpha != 1.f)
                {
                    if (phaseLockState)
                    {
                        FuzzyPV_Sine(XS_current, nHopA, nHopS, ola_coef_syn, frameIndex, phi0_S, psi_S);
                        FuzzyPV_Sine(XT_current, nHopA, nHopS, ola_coef_syn, frameIndex, phi0_T, psi_T);
                        FuzzyPV_Noise(XN_current, nHopA, nHopS, ola_coef_syn, frameIndex, phi0_N, psi_N);
                    }
                }


                // Sine
                phaseVocoder(channel, circBufferSize, outWritePtr, outputCircularBuffer_S, XS_current, fftSizeTSM, nHopS, win_hann_syn, inverseFFT_TSM);

                // Transient
                if (transientPreserveState)// && (resampleRate == 1.0f))
                {
                    transientVocoder(channel, circBufferSize, outWritePtr, outputCircularBuffer_T, tempGrainT_Real, fftSizeTSM, nHopS, win_hann_syn);
                }
                else
                {                    
                    phaseVocoder(channel, circBufferSize, outWritePtr, outputCircularBuffer_T, XT_current, fftSizeTSM, nHopS, win_hann_syn, inverseFFT_TSM);
                }


                // Noise
                phaseVocoder(channel, circBufferSize, outWritePtr, outputCircularBuffer_N, XN_current, fftSizeTSM, nHopS, win_hann_syn, inverseFFT_TSM);


                // Update the output buffer write pointer once the FFT is done 
                outWritePtr = (outWritePtr + nHopS) % circBufferSize;


                frameIndex = frameIndex  + 1;
                if (frameIndex >= nFramesNew)
                {
                    frameIndex = 0;
                }
                instantFrameRatio = (float)frameIndex / (float)nFramesNew;
            }
        }

        // Create a new float array  to store the copy of destBuffer for downsampling for multiresolution stft
        float* destBufferDown = new float[samplesThisTime];
        std::copy(destBuffer, destBuffer + samplesThisTime, destBufferDown);
        juce::dsp::AudioBlock<float> audioBlock(&destBufferDown, (size_t)1, (size_t)samplesThisTime);
        lowpassFilter.process(juce::dsp::ProcessContextReplacing<float>(audioBlock));
        bufferToFill.buffer->copyFrom(1, 0, bufferToFill.buffer->getReadPointer(0), bufferToFill.buffer->getNumSamples());

        for (size_t n = 0; n < samplesThisTime; ++n)
        {
            pushNextSampleIntoFifo_fft(destBuffer[n], destBufferDown[n]);
        }

        delete[] destBufferDown; //delete the allocated memory when done
        // rmsMaster is in dB, the other are in linear scale
        const float rmsSineTemp = calculateRMSVec(sineBuff, samplesThisTime);
        const float rmsTransTemp = calculateRMSVec(transBuff, samplesThisTime);
        const float rmsNoiseTemp = calculateRMSVec(NoiseBuff, samplesThisTime);
        const float rmsMasterTemp = calculateRMS(destBuffer, bufferToFill.buffer->getNumSamples());

        float rmsSineSM = (rmsSineTemp < rmsSine_prev) ? ((1 - 0.92) * rmsSineTemp + 0.92 * rmsSine_prev) : rmsSineTemp;
        float rmsTransSM = (rmsTransTemp < rmsTrans_prev) ? ((1 - 0.92) * rmsTransTemp + 0.92 * rmsTrans_prev) : rmsTransTemp;
        float rmsNoiseSM = (rmsNoiseTemp < rmsNoise_prev) ? ((1 - 0.92) * rmsNoiseTemp + 0.92 * rmsNoise_prev) : rmsNoiseTemp;
        float rmsMasterSM = (rmsMasterTemp < rmsMaster_prev) ? ((1 - 0.92) * rmsMasterTemp + 0.92 * rmsMaster_prev) : rmsMasterTemp;

        rmsMaster = juce::Decibels::gainToDecibels(rmsMasterSM, -72.f);
        rmsSine = juce::Decibels::gainToDecibels(rmsSineSM, -72.f);
        rmsTrans = juce::Decibels::gainToDecibels(rmsTransSM, -72.f);
        rmsNoise = juce::Decibels::gainToDecibels(rmsNoiseSM, -72.f);

        rmsSine_prev = rmsSineSM; rmsTrans_prev = rmsTransSM; rmsNoise_prev = rmsNoiseSM; rmsMaster_prev = rmsMasterSM;



        outputSamplesRemaining -= samplesThisTime;                                         
        outputSamplesOffset += samplesThisTime;                                           
        position += samplesThisTime;                                                       


        if (position == outputCircularBuffer_S.getNumSamples())
            position = 0;
    }
    juce::AudioBuffer<float> tempBuffer(bufferToFill.buffer->getNumChannels(), bufferToFill.numSamples);
    tempBuffer.copyFrom(0, 0, bufferToFill.buffer->getReadPointer(0), bufferToFill.buffer->getNumSamples());
    tempBuffer.applyGain(0, 0, tempBuffer.getNumSamples(), (float)waveZoomAmp.getValue());
    waveViewer.pushBuffer(tempBuffer);
}



void MainComponent::phaseVocoder(int channel, int circBufferSize, int outWritePtr, juce::AudioBuffer<float>& outputCircularBuffer,
    Vec1DComplex& currentFrameSTFT, int fftSize, int hopSize, Vec1D window, juce::dsp::FFT& inverseFFT)
{
    std::vector<std::complex<float>> grainOut(fftSize);
    std::vector<std::complex<float>> tempGrainOut(fftSize);
    std::vector<float> grain(fftSize);
    float ola_coef = ola_norm_coef(window, hopSize);

    tempGrainOut = currentFrameSTFT; // Access the i-th frame from stftBuffer1_L

    inverseFFT.perform(tempGrainOut.data(), grainOut.data(), true);
    for (int x = 0; x < fftSize; ++x)
    {

        grain[x] = grainOut[x].real() * window[x] / ola_coef;


        int circularBufferIndex = (outWritePtr + x) % circBufferSize;
        outputCircularBuffer.addSample(channel, circularBufferIndex, grain[x]);
    }
}


void MainComponent::transientVocoder(int channel, int circBufferSize, int outWritePtr, juce::AudioBuffer<float>& outputCircularBuffer,
    Vec1D& currentFrameTime, int fftSize, int hopSize, Vec1D window)
{
    // no IFFT performed, process only time-domain transient, for repositioning
    float ola_coef = ola_norm_coef(window, hopSize);
    for (int x = 0; x < fftSize; ++x)
    {
        int circularBufferIndex = (outWritePtr + x) % circBufferSize;

        outputCircularBuffer.addSample(channel, circularBufferIndex, currentFrameTime[x] * window[x] / ola_coef);
    }
}

void MainComponent::FuzzyPV_Sine(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi)
{
    size_t NFFT = X.size();
    size_t nBins = (NFFT / 2) + 1;
    //Vec2DComplex Y(nFrames, Vec1DComplex(NFFT));
    Vec1DComplex half_X(nBins);

    int n = frameIndex;

    for (int row = 0; row < nBins; ++row)
    {
        half_X[row] = X[row];
    }

    //Vec1DComplex f(nBins);
    Vec1D r(nBins), r_synth(nBins), phi(nBins);

    std::vector<int> peak_loc(nBins, 0);


    // PV variables
    std::vector<float> omega(nBins);
    for (int k = 0; k < nBins; ++k)
    {
        omega[k] = 2 * j_Pi * k / NFFT;
    }


    // Current frame spectrum

    for (int i = 0; i < nBins; ++i)
    {
        r[i] = std::abs(half_X[i]); // magnitude
        phi[i] = std::arg(half_X[i]); // angle
    }
    // Find spectral peaks
    size_t i = 2;
    size_t nPeaks = 0;
    while (i < nBins - 2)
    {
        if (r[i] > r[i - 1] && r[i] > r[i - 2] && r[i] > r[i + 1] && r[i] > r[i + 2])
        {
            nPeaks++;
            peak_loc[nPeaks - 1] = i;
            i += 3;
        }
        else
        {
            i++;
        }
    }
    // Phase propagation
    if (n == 0)
    {
        // First frame, use analysis phase
        psi = phi;
    }
    else if (nPeaks > 0)
    {
        // Phase locking
        for (size_t i = 0; i < nPeaks; ++i)
        {

            // Find peak phase rotation
            int p = peak_loc[i];
            float h_phase_incr = princArg(phi[p] - phi0[p] - omega[p] * hopSizeAna);
            float inst_freq = omega[p] + h_phase_incr / hopSizeAna;
            float p_phase_rotation = princArg(psi[p] + hopSizeSyn * inst_freq - phi[p]);

            // Find bins around peak
            int bin_low, bin_high;
            if (nPeaks == 1)
            {
                bin_low = 0;
                bin_high = nBins;
            }
            else if (i == 0)
            {
                bin_low = 0;
                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
            }
            else if (i == nPeaks)
            {
                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                bin_high = nBins;
            }
            else
            {
                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
            }
            // Rotate phases according to peak rotation
            for (int j = bin_low; j < bin_high; ++j)
            {
                psi[j] = phi[j] + p_phase_rotation;

            }
        }
    }
    else
    {
        // No peaks found, standard PV processing
        for (size_t i = 0; i < nBins; ++i) {
            float h_phase_incr = princArg(phi[i] - phi0[i] - hopSizeAna * omega[i]);
            float inst_freq = omega[i] + h_phase_incr / hopSizeAna;
            psi[i] = princArg(psi[i] + inst_freq * hopSizeSyn);
        }
    }
    r_synth = r;

    // Update
    for (size_t i = 0; i < nBins; ++i)
    {
        X[i] = r_synth[i] * std::exp(std::complex<float>(0, psi[i]));
        phi0[i] = phi[i];
    }
    for (size_t j = nBins; j < NFFT; ++j)
    {

        X[j] = std::conj(X[NFFT - j]);
    }


}

void MainComponent::FuzzyPV_Noise(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi)
{
    size_t NFFT = X.size();
    size_t nBins = (NFFT / 2) + 1;
    //Vec2DComplex Y(nFrames, Vec1DComplex(NFFT));
    Vec1DComplex half_X(nBins);
    Vec1D half_X_mag(nBins);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.5, 0.5);


    int n = frameIndex;

    for (int row = 0; row < nBins; ++row)
    {
        half_X[row] = X[row];
        half_X_mag[row] = std::tanh(std::abs(X[row])); // using tanh() cuz it's easier than /max(abs())
    }

    //Vec1DComplex f(nBins);
    Vec1D r(nBins), r_synth(nBins), phi(nBins);

    std::vector<int> peak_loc(nBins, 0);

    float noise_coef = 1 / 4 * std::tanh(4 * (hopSizeSyn / hopSizeAna - 1.5)) + 1;

    // PV variables
    std::vector<float> omega(nBins);
    for (int k = 0; k < nBins; ++k)
    {
        omega[k] = 2 * j_Pi * k / NFFT;
    }


    // Current frame spectrum

    for (int i = 0; i < nBins; ++i)
    {
        r[i] = std::abs(half_X[i]); // magnitude
        phi[i] = std::arg(half_X[i]); // angle
    }
    // Find spectral peaks
    size_t i = 2;
    size_t nPeaks = 0;
    while (i < nBins - 2)
    {
        if (r[i] > r[i - 1] && r[i] > r[i - 2] && r[i] > r[i + 1] && r[i] > r[i + 2])
        {
            nPeaks++;
            peak_loc[nPeaks - 1] = i;
            i += 3;
        }
        else
        {
            i++;
        }
    }
    // Phase propagation
    if (n == 0)
    {
        // First frame, use analysis phase
        psi = phi;
    }
    else if (nPeaks > 0)
    {
        // Phase locking
        for (size_t i = 0; i < nPeaks; ++i)
        {

            // Find peak phase rotation
            int p = peak_loc[i];
            float h_phase_incr = princArg(phi[p] - phi0[p] - omega[p] * hopSizeAna);
            float inst_freq = omega[p] + h_phase_incr / hopSizeAna;
            float p_phase_rotation = princArg(psi[p] + hopSizeSyn * inst_freq - phi[p]);

            // Find bins around peak
            int bin_low, bin_high;
            if (nPeaks == 1)
            {
                bin_low = 0;
                bin_high = nBins;
            }
            else if (i == 0)
            {
                bin_low = 0;
                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
            }
            else if (i == nPeaks)
            {
                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                bin_high = nBins;
            }
            else
            {
                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
            }
            // Rotate phases according to peak rotation
            for (int j = bin_low; j < bin_high; ++j)
            {
                psi[j] = phi[j] + p_phase_rotation;
                // Phase randomization
                auto Rn = half_X_mag[j]; // noisiness
                auto An = (std::tanh(4 * (Rn - 1)) + 1) * noise_coef;
                auto phase_noise_value = j_Pi * An * (dis(gen) - 0.5); // dis(gen) is rand(1,1)
                psi[j] = princArg(psi[j] + phase_noise_value);

            }
        }
    }
    else
    {
        // No peaks found, standard PV processing
        for (size_t i = 0; i < nBins; ++i) {
            float h_phase_incr = princArg(phi[i] - phi0[i] - hopSizeAna * omega[i]);
            float inst_freq = omega[i] + h_phase_incr / hopSizeAna;
            psi[i] = princArg(psi[i] + inst_freq * hopSizeSyn);
        }
    }
    r_synth = r;

    // Update
    for (size_t i = 0; i < nBins; ++i)
    {
        X[i] = r_synth[i] * std::exp(std::complex<float>(0, psi[i]));
        phi0[i] = phi[i];
    }
    for (size_t j = nBins; j < NFFT; ++j)
    {

        X[j] = std::conj(X[NFFT - j]);
    }
}


///------------------------------------------------------------------------------------------------------------
//void MainComponent::FuzzyPV_Noise(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi, Vec1D& Rn)
//{
//    size_t NFFT = X.size();
//    size_t nBins = (NFFT / 2) + 1;
//    //Vec2DComplex Y(nFrames, Vec1DComplex(NFFT));
//    Vec1DComplex half_X(nBins);
//    Vec1D half_X_mag(nBins);
//
//    std::random_device rd;
//    std::mt19937 gen(rd());
//    std::uniform_real_distribution<float> dis(-0.5, 0.5);
//
//
//    int n = frameIndex;
//
//    for (int row = 0; row < nBins; ++row)
//    {
//        half_X[row] = X[row];
//        half_X_mag[row] = std::tanh(std::abs(X[row])); // using tanh() cuz it's easier than /max(abs())
//    }
//
//    //Vec1DComplex f(nBins);
//    Vec1D r(nBins), r_synth(nBins), phi(nBins);
//
//    std::vector<int> peak_loc(nBins, 0);
//
//    float noise_coef = 1 / 4 * std::tanh(4 * (hopSizeSyn / hopSizeAna - 1.5)) + 1;
//
//    // PV variables
//    std::vector<float> omega(nBins);
//    for (int k = 0; k < nBins; ++k)
//    {
//        omega[k] = 2 * j_Pi * k / NFFT;
//    }
//
//
//    // Current frame spectrum
//
//    for (int i = 0; i < nBins; ++i)
//    {
//        r[i] = std::abs(half_X[i]); // magnitude
//        phi[i] = std::arg(half_X[i]); // angle
//    }
//    // Find spectral peaks
//    size_t i = 2;
//    size_t nPeaks = 0;
//    while (i < nBins - 2)
//    {
//        if (r[i] > r[i - 1] && r[i] > r[i - 2] && r[i] > r[i + 1] && r[i] > r[i + 2])
//        {
//            nPeaks++;
//            peak_loc[nPeaks - 1] = i;
//            i += 3;
//        }
//        else
//        {
//            i++;
//        }
//    }
//    // Phase propagation
//    if (n == 0)
//    {
//        // First frame, use analysis phase
//        psi = phi;
//    }
//    else if (nPeaks > 0)
//    {
//        // Phase locking
//        for (size_t i = 0; i < nPeaks; ++i)
//        {
//
//            // Find peak phase rotation
//            int p = peak_loc[i];
//            float h_phase_incr = princArg(phi[p] - phi0[p] - omega[p] * hopSizeAna);
//            float inst_freq = omega[p] + h_phase_incr / hopSizeAna;
//            float p_phase_rotation = princArg(psi[p] + hopSizeSyn * inst_freq - phi[p]);
//
//            // Find bins around peak
//            int bin_low, bin_high;
//            if (nPeaks == 1)
//            {
//                bin_low = 0;
//                bin_high = nBins;
//            }
//            else if (i == 0)
//            {
//                bin_low = 0;
//                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
//            }
//            else if (i == nPeaks)
//            {
//                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
//                bin_high = nBins;
//            }
//            else
//            {
//                bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
//                bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
//            }
//            // Rotate phases according to peak rotation
//            for (int j = bin_low; j < bin_high; ++j)
//            {
//                psi[j] = phi[j] + p_phase_rotation;
//                // Phase randomization
//                /*auto _Rn = std::min(std::max(Rn[j], 0.0f), 1.0f);
//                auto An = (std::tanh(4 * (_Rn - 1)) + 1) * noise_coef;*/
//                auto RnA = half_X_mag[j]; // noisiness                
//                auto An = (std::tanh(4 * (RnA - 1)) + 1) * noise_coef;
//                auto phase_noise_value = j_Pi * An * (dis(gen) - 0.5); // dis(gen) is rand(1,1)
//                psi[j] = princArg(psi[j] + phase_noise_value);
//            }
//        }
//    }
//    else
//    {
//        // No peaks found, standard PV processing
//        for (size_t i = 0; i < nBins; ++i) {
//            float h_phase_incr = princArg(phi[i] - phi0[i] - hopSizeAna * omega[i]);
//            float inst_freq = omega[i] + h_phase_incr / hopSizeAna;
//            psi[i] = princArg(psi[i] + inst_freq * hopSizeSyn);
//        }
//    }
//    r_synth = r;
//
//    // Update
//    for (size_t i = 0; i < nBins; ++i)
//    {
//        X[i] = r_synth[i] * std::exp(std::complex<float>(0, psi[i]));
//        phi0[i] = phi[i];
//    }
//    for (size_t j = nBins; j < NFFT; ++j)
//    {
//
//        X[j] = std::conj(X[NFFT - j]);
//    }
//}
//

void MainComponent::timerCallback()
{

        if (nextFFTBlockReady_visual_fft)
        {
            drawNextFrameOfSpectrum();
            nextFFTBlockReady_visual_fft = false;
            repaint();
        }
        
        if (state != Stopped)
        {
            float instantFrameRatio_temp = instantFrameRatio * 0.9f + instantFrameRatio_prev * 0.1f;
            progressBarSlider.setValue(instantFrameRatio_temp);
            instantFrameRatio_prev = instantFrameRatio_temp;

        }
            
        else
        {
            progressBarSlider.setValue(0.f);
            instantFrameRatio_prev = 0.f;
        }
        
            
}


void MainComponent::sliderValueChanged(juce::Slider* slider) 
{
    if (slider == &progressBarSlider)
    {
        if (slider->isMouseButtonDown())
        {
            // The user is currently dragging the slider
            // Update frameIndex immediately without affecting the audio playback
            frameIndex = static_cast<int>(progressBarSlider.getValue() * nFrames_prev);
        }
    }
    
}

juce::Colour myColorMap(float value)
{

    // Create a ColourGradient to define your custom color map
    juce::ColourGradient gradient;
    //gradient.addColour(0.0, juce::Colours::black);
    gradient.addColour(0.0, juce::Colours::transparentBlack);
    gradient.addColour(0.001, juce::Colours::blue);
    gradient.addColour(0.1, juce::Colours::cyan);
    gradient.addColour(0.15, juce::Colours::green);
    gradient.addColour(0.2, juce::Colours::greenyellow);
    gradient.addColour(0.3, juce::Colours::yellow);
    gradient.addColour(0.4, juce::Colours::orange);
    gradient.addColour(0.5, juce::Colours::darkorange);
    gradient.addColour(0.7, juce::Colours::red);
    gradient.addColour(0.9, juce::Colours::darkred);


    // Get the interpolated color from the gradient based on the normalized value
    juce::Colour interpolatedColor = gradient.getColourAtPosition(value);

    // Return the interpolated color
    return interpolatedColor;

}

void MainComponent::pushNextSampleIntoFifo_fft(float sampleFFtAndSpecgram, float sampleSpecgramDown) noexcept
{
    if (fifoIndex_visual_fft == fftSize_visual_fft)               
    {
        if (!nextFFTBlockReady_visual_fft)           
        {
            juce::zeromem(fftData_visual_fft, sizeof(fftData_visual_fft));
            juce::zeromem(fftData_visual_specgram, sizeof(fftData_visual_specgram));
            juce::zeromem(fftData_visual_specgramDown, sizeof(fftData_visual_specgramDown));

            memcpy(fftData_visual_fft, fifo_visual_fft, sizeof(fifo_visual_fft));
            memcpy(fftData_visual_specgram, fifo_visual_specgram, sizeof(fifo_visual_specgram));
            memcpy(fftData_visual_specgramDown, fifo_visual_specgramDown, sizeof(fifo_visual_specgramDown));

            nextFFTBlockReady_visual_fft = true;
        }
        //fifoIndex_visual_fft = 0;
        const int hopSize = fftSize_visual_fft /64; 
        for (int i = 0; i < fftSize_visual_fft - hopSize; ++i)
        {
            fifo_visual_fft[i] = fifo_visual_fft[i + hopSize];
            fifo_visual_specgram[i] = fifo_visual_specgram[i + hopSize];
            fifo_visual_specgramDown[i] = fifo_visual_specgramDown[i + hopSize];
        }
            
        fifoIndex_visual_fft = fftSize_visual_fft - hopSize;
    }
    auto commonFifoIndex = fifoIndex_visual_fft++;
    fifo_visual_fft[commonFifoIndex] = sampleFFtAndSpecgram;
    fifo_visual_specgram[commonFifoIndex] = sampleFFtAndSpecgram;
    fifo_visual_specgramDown[commonFifoIndex] = sampleSpecgramDown;
}



void MainComponent::drawNextFrameOfSpectrum()
{
    

    // For spectrogram
    Vec1DComplex fftData_Spec_Norm_in(fftSize_visual_fft); // 2^14
    Vec1DComplex fftData_Spec_Norm_out(fftSize_visual_fft); // 2^14
    
    int decimationFactor = 4;
    int decimatedArraylength = ((fftSize_visual_fft - 1) / decimationFactor + 1);
    Vec1DComplex fftData_Spec_Down(fftSize_visual_fft); 
    Vec1DComplex fftData_Spec_Deci_in(fftSize_visual_fft, { 0.0f, 0.0f }); // 2^14, init with zeros cuz we need zero-padding  anyway
    Vec1DComplex fftData_Spec_Deci_out(fftSize_visual_fft); // 2^14

    auto win_hann_visual = makeHannWindow(fftSize_visual_fft);
    float winScale = 0.2499695; // half of average of hann window, divide this to the data to normalize
    // load data from fifo buffer to a complex vector
    
    for (int i = 0; i < fftSize_visual_fft; ++i) {
        fftData_Spec_Norm_in[i] = std::complex<float>(fftData_visual_specgram[i] * win_hann_visual[i], 0.0f); // simultaneously applying window
        fftData_Spec_Down[i] = std::complex<float>(fftData_visual_specgramDown[i] * win_hann_visual[i], 0.0f); // simultaneously applying window
    }

    for (size_t i = 0; i < decimatedArraylength; ++i) {
        size_t sourceIndex = i * decimationFactor; // Calculate source index

        if (sourceIndex < fftSize_visual_fft) {
            fftData_Spec_Deci_in[i] = (fftData_Spec_Down[sourceIndex]);
            ;
        }
    }


    forwardFFT_visual_fft.perform(fftData_Spec_Norm_in.data(), fftData_Spec_Norm_out.data(), false);
    forwardFFT_visual_fft.perform(fftData_Spec_Deci_in.data(), fftData_Spec_Deci_out.data(), false);

    auto mindB = -72.0f;
    auto maxdB = 0.0f;

    // // ----------------------------- For FFT analyzer -----------------------------------// //
    for (int i = 0; i < scopeSize_visual_fft; ++i)
    {
        auto scaledMag = (std::abs(fftData_Spec_Norm_out[i]) / fftSize_visual_fft) / winScale;
        auto level = juce::jmap(juce::jlimit(mindB, maxdB, mag2dB(scaledMag)), mindB, maxdB, 0.0f, 1.0f);

        scopeData_visual_fft[i] = level;
    }
    // Smooth the spectrum data using a 3-point average
    for (int i = 1; i < scopeSize_visual_fft - 1; ++i)
    { 
        scopeData_visual_fft[i] = mag2dB((dB2mag(scopeData_visual_fft[i - 1]) + dB2mag(scopeData_visual_fft[i]) + dB2mag(scopeData_visual_fft[i + 1])) / 3.0f);
    }
    // // ----------------------------- DONE FFT analyzer -----------------------------------// //

    // // ----------------------------- For Spectrogram BOTH resolution -----------------------------------// //

    // for spectrogram
    auto rightHandEdge = spectrogramImage.getWidth() - 1;
    auto imageHeight = spectrogramImage.getHeight();
    // for spectrogram time axis ticks
    auto rightHandEdgeTime = spectrogramTimeImage.getWidth() - 1;
    auto imageHeightTime = spectrogramTimeImage.getHeight();

    // first, shuffle our image leftwards by 1 pixel..
    spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);
    spectrogramImageDown.moveImageSection(0, 0, 1, 0, rightHandEdge, spectrogramImageDown.getHeight());
    spectrogramTimeImage.moveImageSection(0, 0, 1, 0, rightHandEdgeTime, imageHeightTime);
    float minFreq = 10.0f;
    float maxFreq = fftFs/2;
    Vec1D levelNormal , levelDown;
    int startPointN = std::round(10 * ((fftSize_visual_fft / 2 ) / (fftFs / 2))); // compute the starting index for 10 Hz
    int startPointD = std::round(10 * ((fftSize_visual_fft / 2 ) / (fftFs / 8))); // compute the starting index for 10 Hz
    for (auto y = 0; y < spectrogramImage.getHeight(); ++y)
    {
        // Map the y-coordinate to a normalized value in the range [0, 1]
        float normY = juce::jmap(static_cast<float>(y), 1.0f, static_cast<float>(imageHeight - 1), 0.0f, 1.0f);
        // Map the normalized y-coordinate to the frequency range from 0 Hz to Fs/2 (logarithmically)
        float frequency = juce::jmap(normY, 0.0f, 1.0f, 1.0f, 0.0f);
        frequency = std::pow(10.0f, juce::jmap(frequency, 0.0f, 1.0f, std::log10(10.f), std::log10(fftFs / 2.0f)));
        //DBG("frequency: " << frequency);
        auto fftDataIndex = juce::jlimit(startPointN, fftSize_visual_fft / 2 , static_cast<int>(frequency / fftFs * fftSize_visual_fft));// Map the frequency to the FFT data index
        //DBG("fftDataIndex: " << fftDataIndex);
        auto scaledMagN = (std::abs(fftData_Spec_Norm_out[fftDataIndex]) / fftSize_visual_fft) / winScale;
        auto levelN = juce::jmap(juce::jlimit(mindB, maxdB, mag2dB(scaledMagN)), mindB, maxdB, 0.0f, 1.0f);


        float normYDown = juce::jmap(static_cast<float>(y), 1.0f, static_cast<float>(spectrogramImageDown.getHeight() - 1), 0.0f, 1.0f);
        float frequencyDown = juce::jmap(normYDown, 0.0f, 1.0f, 1.0f, 0.0f);
        frequencyDown = std::pow(10.0f, juce::jmap(frequencyDown, 0.0f, 1.0f, std::log10(10.f), std::log10(fftFs / 8)));
        auto fftDataIndexDown = juce::jlimit(startPointD, fftSize_visual_fft / 2 , static_cast<int>(frequencyDown / (fftFs / 4) * fftSize_visual_fft));
        auto scaledMagD = (std::abs(fftData_Spec_Deci_out[fftDataIndexDown]) / decimatedArraylength) / winScale;
        //auto scaledMagD = (std::abs(fftData_Spec_Deci_out[fftDataIndexDown]) / fftSize_visual_fft) / winScale;
        auto levelD = juce::jmap(juce::jlimit(mindB, maxdB, mag2dB(scaledMagD)), mindB, maxdB, 0.0f, 1.0f);
        
        spectrogramImage.setPixelAt(rightHandEdge, y, myColorMap(levelN));
        if (y < spectrogramImageDown.getHeight())
            spectrogramImageDown.setPixelAt(rightHandEdge, y, myColorMap(levelD));


        
    }
       
    if (specgramSecCountReady)
    {
        // Clear the spectrogramTimeImage to white only once every 1 second
        for (auto y = 0; y < imageHeightTime; ++y)
        {
            spectrogramTimeImage.setPixelAt(rightHandEdgeTime, y, juce::Colour::fromHSL(1.f, 1.0f, 1.f, 1.0f));
        }
        specgramSecCountReady = false;
    }
    else
    {
        // Fill the spectrogramTimeImage with black for all other times
        for (auto y = 0; y < imageHeightTime; ++y)
        {
            spectrogramTimeImage.setPixelAt(rightHandEdgeTime, y, juce::Colours::transparentBlack);
            if (y == std::floor(imageHeightTime / 2))
                spectrogramTimeImage.setPixelAt(rightHandEdgeTime, y, juce::Colours::silver);
        }
    }
}
  
// Custom log-scale mapping function
float mapToLogScale(float value, float minRange, float maxRange)
{
    // Ensure the value is within the range
    value = juce::jlimit(minRange, maxRange, value);

    // Map the value to the logarithmic scale
    float minLog = std::log10(minRange);
    float maxLog = std::log10(maxRange);
    float logValue = std::log10(value);

    return (logValue - minLog) / (maxLog - minLog);
}


void MainComponent::drawFrame(juce::Graphics& g, int x, int y, int width, int height)
{
    float minFreq = 10.0f;
    float maxFreq = fftFs/2;
    int startPointN = std::floor(minFreq * ((scopeSize_visual_fft) / (fftFs / 2))); // compute the starting index for 10 Hz
    for (int i = 1; i < scopeSize_visual_fft; ++i)
    {
        // Compute the frequency value in log scale for each index i
        float frequency = juce::jmap(i - 1, startPointN, scopeSize_visual_fft - 1, (int)minFreq, (int)maxFreq);

        // Map the frequency to the normalized x-coordinate using the log scale mapping
        float normX = mapToLogScale(frequency, minFreq, maxFreq);

        // Calculate the actual x-coordinate within the drawing area
        float x1 = x + normX * width;
        float x2 = x + mapToLogScale(juce::jmap(i, startPointN, scopeSize_visual_fft - 1, (int)minFreq, (int)maxFreq), minFreq, maxFreq) * width;

        // Cast y and height to float before using jmap
        float y1 = juce::jmap(scopeData_visual_fft[i - 1], 0.0f, 1.0f, static_cast<float>(y) + static_cast<float>(height), static_cast<float>(y));
        float y2 = juce::jmap(scopeData_visual_fft[i], 0.0f, 1.0f, static_cast<float>(y) + static_cast<float>(height), static_cast<float>(y));

        g.drawLine(x1, y1, x2, y2, 1.25f);
    }
}



//==============================================================================
//----- PRIVATE

void MainComponent::openButtonClicked()
{
    playButton.setImages(false, true, true, playButtonNormal, 1.0f, {}, playButtonNormal, 0.7f, {}, playButtonDown, 1.0f, {});
    playButton.setEnabled(false);
    stopButton.setEnabled(false);
    shutdownAudio();
    setAudioChannels(0, 2);
    auto* device = deviceManager.getCurrentAudioDevice();
    systemFs = device->getCurrentSampleRate();
    shutdownAudio();  
    // clear things out
    resetpitchTSMButtonClicked();
    // clear visualization
    clearVisualStuff();
    

    chooser = std::make_unique<juce::FileChooser>("Select an Audio File...",
        juce::File{}, "* .wav; *.mp3; *.flac");

    statusLabel.setText("Loading & Processing", juce::dontSendNotification);
    fileNameLable.setText(" ", juce::dontSendNotification);

    auto chooserFlags = juce::FileBrowserComponent::openMode
        | juce::FileBrowserComponent::canSelectFiles;


    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();

            if (file == juce::File{})
                return;

            std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file)); 
            //numOutChannels = (int)reader->numChannels;
            numOutChannels = 2;
            if (reader.get() != nullptr)
            {

                float duration = (float)reader->lengthInSamples / reader->sampleRate;               // [3]
                float limitDuration = 20; // limit to be 30 sec long
                fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);  // [4]
                

                reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0,  true, true);              
                
                fileNameLable.setText("File Name: " + juce::String(file.getFileName()), juce::dontSendNotification);

                auto* mBuffer = &fileBuffer;
                auto mNumChannels = mBuffer->getNumChannels();
                auto mNumSamples = mBuffer->getNumSamples();
                
                size_t numSamples = mBuffer->getNumSamples();
                float* channelDataL = mBuffer->getWritePointer(0);

                if (duration > limitDuration)
                {
                    int limitNumSample = std::floor(limitDuration * (reader->sampleRate));
                    // Make sure the limit is within the bounds of the array
                    limitNumSample = std::min(limitNumSample, static_cast<int>(numSamples));

                    // Limit/trim the array to limitNumSample
                    numSamples = limitNumSample;
                }

                // Perform STFT
                int winSize1 = fftSize1;
                int sizeChannelDataL = numSamples;
                Vec1D channelDataVec(channelDataL, channelDataL + sizeChannelDataL);
                Vec1D xL_old;
                int kDownfactor = reader->sampleRate;
                int kUpfactor = systemFs;

                resample<float>(kUpfactor, kDownfactor, channelDataVec, xL_old);

                Vec1D xL = zeroPadTime(xL_old, winSize1);

                // clear xL_old
                xL_old = Vec1D();

                normalizeSignal(xL);

                int Fs = systemFs;


                auto numRows1 = fftSize1 / 2 + 1;                                    // nBins
                auto numElements1 = fftSize1;                                    // double sided fft
                auto numCols1 = std::floor((xL.size() - fftSize1) / hopSize1 + 1); // nFrames


                Vec1D win_hann1(fftSize1);
                for (int i = 0; i < (int)fftSize1; ++i)
                {
                    win_hann1[i] = 0.5f - 0.5f * std::cos(2.0f * j_Pi * (float)i / (float(fftSize1 - 1)));
                }

                stftBuffer1_L.clear();

                stftBuffer1_L = doForwardSTFT(stftBuffer1_L, fftSize1, hopSize1, win_hann1, xL, forwardFFT1, numCols1, numElements1);


                auto abs_stftBuffer1_L = makeAbsSTFT(stftBuffer1_L, numCols1, numRows1);


                ////-------------------------------- MEDIAN FILTER 1 -------------------------------//
                //// 
                //// ============== Vertical median filtering (along Frequency Bins)
               

                
                int filter_length_f = 500;// in Hz
                int nMedianV1 = std::round(filter_length_f * (int)fftSize1 / Fs);
                //auto X_V_median = medfiltVerticalAVX2(abs_stftBuffer1_L, nMedianV1);
                auto X_V_median = medfiltVertical(abs_stftBuffer1_L, nMedianV1);

                ////// ============== Horizontal median filtering  (along Time Frames) =========
                /////

                float filter_length_t = 200e-3;// in ms
                int nMedianH1 = std::round(filter_length_t * Fs / hopSize1);
                auto X_H_median = medfiltHorizontal(abs_stftBuffer1_L, nMedianH1);
                //auto X_H_median = medfiltHorizontalAVX2(abs_stftBuffer1_L, nMedianH1);

                auto Rt = transientness(X_H_median, X_V_median);
                float G1 = 0.8;
                float G2 = 0.7;
                
                std::tie(S_matrix1, T_matrix1, N_matrix1) = decSTN(Rt, G2, G1);


                XS = applyMask(S_matrix1, stftBuffer1_L);
                

                yS = doInverseSTFT(XS, fftSize1, hopSize1, numCols1, win_hann1, inverseFFT1); //ys
                int outLen = yS.size();

                
                // Make Residual Mask   
                Vec2D Res_matrix(S_matrix1.size(), std::vector<float>(S_matrix1[0].size()));
                for (size_t row = 0; row < XS.size(); ++row) {
                    for (size_t col = 0; col < XS[0].size(); ++col) {
                        Res_matrix[row][col] = T_matrix1[row][col] + N_matrix1[row][col];                        
                    }
                }
                auto XRes = applyMask(Res_matrix, stftBuffer1_L);
                auto yRes = doInverseSTFT(XRes, fftSize1, hopSize1, numCols1, win_hann1, inverseFFT1); //yResidual

                //clear stftBuffer1_L, XS, XRes, X_median  and other round 1 stuff
                stftBuffer1_L = Vec2DComplex();
                XS, XRes = Vec2DComplex();
                X_H_median, X_V_median, Rt, abs_stftBuffer1_L = Vec2D();
                S_matrix1, T_matrix1, N_matrix1, Res_matrix = Vec2D();
                xL, win_hann1 = Vec1D();

                //--------------------------------- Round 2 ----------------------------------//
                // Perform STFT
                int winSize2 = fftSize2;
                auto numRows2 = fftSize2 / 2 + 1;                                    // nBins
                auto numElements2 = fftSize2;                                    // double sided fft
                auto numCols2 = std::floor((yRes.size() - fftSize2) / hopSize2 + 1); // nFrames
                

                stftBuffer2_L.resize(numElements2, std::vector<std::complex<float>>(numCols2));


                std::vector<float> win_hann2(winSize2);
                for (int i = 0; i < (int)winSize2; ++i)
                {
                    win_hann2[i] = 0.5f - 0.5f * std::cos(2.0f * j_Pi * (float)i / (float(winSize2 - 1)));
                }

                stftBuffer2_L.clear();

                stftBuffer2_L = doForwardSTFT(stftBuffer2_L, fftSize2, hopSize2, win_hann2, yRes, forwardFFT2, numCols2, numElements2);
                auto abs_stftBuffer2_L = makeAbsSTFT(stftBuffer2_L, numCols2, numRows2);


                int nMedianV2 = std::round(filter_length_f * fftSize2 / Fs);
                int nMedianH2 = std::round(filter_length_t * Fs / hopSize2);


                auto X_V_median2 = medfiltVertical(abs_stftBuffer2_L, nMedianV2);
                auto X_H_median2 = medfiltHorizontal(abs_stftBuffer2_L, nMedianH2);

                //auto X_V_median2 = medfiltVerticalAVX2(abs_stftBuffer2_L, nMedianV2);
                //auto X_H_median2 = medfiltHorizontalAVX2(abs_stftBuffer2_L, nMedianH2);
                auto Rt2 = transientness(X_H_median2, X_V_median2);
                float G1_2 = 0.85;
                float G2_2 = 0.75;
                std::vector<std::vector<float>> S_matrix2, T_matrix2, N_matrix2;
                std::tie(S_matrix2, T_matrix2, N_matrix2) = decSTN(Rt2, G2_2, G1_2);
                DBG("============================== Done decSTN 2nd ==============================");
                // Make Noise Mask from 2nd round (S+N) or (1-T)
                for (size_t row = 0; row < T_matrix2.size(); ++row) {
                    for (size_t col = 0; col < T_matrix2[0].size(); ++col) {
                        N_matrix2[row][col] = 1.0 - T_matrix2[row][col] ;
                    }
                }
                XT = applyMask(T_matrix2, stftBuffer2_L);
                XN = applyMask(N_matrix2, stftBuffer2_L);
                yT = doInverseSTFT(XT, fftSize2, hopSize2, numCols2, win_hann2, inverseFFT2); //yt
                yN = doInverseSTFT(XN, fftSize2, hopSize2, numCols2, win_hann2, inverseFFT2); //yn

                //clear stftBuffer1_L, XS, XRes, X_median  and other round 1 stuff
                stftBuffer2_L = Vec2DComplex();
                
                X_H_median2, X_V_median2, Rt2, abs_stftBuffer2_L = Vec2D();
                S_matrix2, T_matrix2, N_matrix2 = Vec2D();
                yRes = Vec1D();


                win_hann_syn.resize(nWin_syn);
                for (int i = 0; i < (int)nWin_syn; ++i)
                {
                    win_hann_syn[i] = 0.5f - 0.5f * std::cos(2.0f * j_Pi * (float)i / (float(nWin_syn - 1)));
                }

                
                // initialize for no stretch
                decSigLen = yS.size();
                float alpha = tsmSlider.getValue();
                int nHopS = nWin_syn / 8;
                int nHopA = std::round(nHopS / alpha);
                float ola_coef_syn = ola_norm_coef(win_hann_syn, nHopS);

                auto nRows_TSM = fftSizeTSM;                                       // double sided fft
                auto nCols_TSM = std::floor((decSigLen - fftSizeTSM) / nHopA + 1); // nFrames
                nFrames_prev = nCols_TSM; // assignment original nFrames
                
                
                std::vector<float> yT_abs(decSigLen);
                std::transform(yT.begin(), yT.end(), yT_abs.begin(), [](float value) {
                    return std::abs(value);
                    });

                peakLocs = findPeaks(yT_abs, 4096, 0.1f * max_abs(yT_abs));


                transientReposition(yT_tsm, yT, peakLocs, 1, nCols_TSM, nHopS, nWin_syn);

                for (size_t i = 0; i < yT.size(); ++i)
                {
                    yN[i] = yN[i] +( yT[i] - yT_tsm[i]);
                }
                transientReposition(yT_tsm, yT, peakLocs, alpha, nCols_TSM, nHopS, nWin_syn);

                // prepare things needed for getnextblock
                position = 0;    
                frameIndex = 0;

                yS_og = yS;
                yT_og = yT;
                yN_og = yN;
                decSigLen_og = decSigLen;

                RnVec = reconstructOLA(XN, fftSize2, hopSize2, numCols2, win_hann2);
                XT, XN = Vec2DComplex();
                // Once the pre-processing is done, update the label text
                // 
                //to ensure that the buttons display the correct state when a new file is loaded
                if ((state == Paused) || (state == Playing))
                {
                    changeState(Stopped); // Change state to Stopped
                }

                statusLabel.setText("Ready!", juce::dontSendNotification);
                playButton.setEnabled(true);

                // Clear window
                win_hann1, win_hann2, yT_abs = Vec1D();


            }
        });
    
}

void MainComponent::saveButtonClicked()
{
    statusLabel.setText("Saving ...", juce::dontSendNotification);

    if ((state != Playing))
    {
        backButtonClicked();
        playButtonClicked();
    }

    chooser = std::make_unique<juce::FileChooser>("Save an Audio File...",
        juce::File{}, "*.wav");



    auto chooserFlags = juce::FileBrowserComponent::saveMode
        | juce::FileBrowserComponent::canSelectFiles;

    chooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
        {
            if (fc.getResult().exists())
            {
                fileToSave = fc.getResult();
                fileOutputStream = std::make_unique<juce::FileOutputStream>(fileToSave);

                // Set the number of samples you want to record for saving
                samplesToRecord = decSigLen; // Adjust as needed

                samplesRecorded = 0; // Initialize the recorded samples counter
                backButtonClicked();
            }
            else
            {
                statusLabel.setText("Saving canceled.", juce::dontSendNotification);
            }
        });
}


void MainComponent::buttonClicked(juce::Button* button) 
{
    // Handle button clicks here
    if (button == &sineButton)
    {
        sineButtonState = !sineButtonState; // Toggle the state
        sineButton.setImages(false, true, true, sineButtonState ? sineButtonOn : sineButtonOff,
            1.0f, {}, sineButtonState ? sineButtonOn : sineButtonOff,
            0.7f, {}, sineButtonState ? sineButtonOn : sineButtonOff,
            0.5f, {});
        
    }
    else if (button == &transientButton)
    {
        transientButtonState = !transientButtonState; // Toggle the state
        transientButton.setColour(juce::TextButton::buttonColourId, transientButtonState ? juce::Colours::darkgreen : juce::Colours::lightgrey);
        transientButton.setImages(false, true, true, transientButtonState ? transButtonOn : transButtonOff,
            1.0f, {}, transientButtonState ? transButtonOn : transButtonOff,
            0.7f, {}, transientButtonState ? transButtonOn : transButtonOff,
            0.5f, {});
    }
    else if (button == &noiseButton)
    {
        noiseButtonState = !noiseButtonState; // Toggle the state
        noiseButton.setColour(juce::TextButton::buttonColourId, noiseButtonState ? juce::Colours::darkgreen : juce::Colours::lightgrey);
        noiseButton.setImages(false, true, true, noiseButtonState ? noiseButtonOn : noiseButtonOff,
            1.0f, {}, noiseButtonState ? noiseButtonOn : noiseButtonOff,
            0.7f, {}, noiseButtonState ? noiseButtonOn : noiseButtonOff,
            0.5f, {});
    }
    else if (button == &masterButton)
    {
        masterButtonState = !masterButtonState; // Toggle the state
        masterButton.setColour(juce::TextButton::buttonColourId, masterButtonState ? juce::Colours::darkgreen : juce::Colours::lightgrey);
        masterButton.setImages(false, true, true, masterButtonState ? masterButtonOn : masterButtonOff,
            1.0f, {}, masterButtonState ? masterButtonOn : masterButtonOff,
            0.7f, {}, masterButtonState ? masterButtonOn : masterButtonOff,
            0.5f, {});
    }
    else if (button == &transientPreserveButton)
    {
        transientPreserveState = !transientPreserveState;
        transientPreserveButton.setColour(juce::TextButton::buttonColourId, transientPreserveState ? juce::Colours::darkgreen : juce::Colours::lightgrey);
        transientPreserveButton.setImages(false, true, true, transientPreserveState ? transPreserveButtonActive : transPreserveButtonNormal,
            1.0f, {}, transientPreserveState ? transPreserveButtonActive : transPreserveButtonNormal,
            0.7f, {}, transPreserveButtonDown,
            0.9f, {});
    }
    else if (button == &phaseLockButton)
    {
        phaseLockState = !phaseLockState;
        phaseLockButton.setColour(juce::TextButton::buttonColourId, phaseLockState ? juce::Colours::darkgreen : juce::Colours::lightgrey);

        phaseLockButton.setImages(false, true, true, phaseLockState ? pvButtonActive : pvButtonNormal,
            1.0f, {}, phaseLockState ? pvButtonActive : pvButtonNormal,
            0.7f, {}, pvButtonDown,
            0.9f, {});
    }

    else if (button == &hiResButton)
    {
        hiResState = !hiResState;
        hiResButton.setImages(false, true, true, hiResState ? hiresButtonActive : hiresButtonNormal,
            1.0f, {}, hiResState ? hiresButtonActive : hiresButtonNormal,
            0.7f, {}, hiresButtonActive,
            0.9f, {});
    }

    
}



void MainComponent::matchTSMButtonClicked() 
{
    //tsmSlider.setValue(1 / resampleSlider.getValue());
    tsmSlider.setValue(resampleSlider.getValue());
    resampleMatch = true;
}
void MainComponent::resetpitchTSMButtonClicked()
{
    tsmSlider.setValue(1.0f);
    resampleSlider.setValue(1.0f);
}

void MainComponent::clearVisualStuff()
{
    // clear waveform viewer
    waveViewer.clear();


    // Clear FFT spectrum analyzer
    // Use std::memset to set the entire array to 0
    std::memset(scopeData_visual_fft, 0, sizeof(scopeData_visual_fft));
    repaint();
    // clear spectrogram Image
    for (auto x = 0; x < spectrogramImage.getWidth(); ++x)
    {
        for (auto y = 0; y < spectrogramImage.getHeight(); ++y)
        {
            //spectrogramImage.setPixelAt(x, y, juce::Colour::fromHSL(0, 1.0f, 0, 1.0f));
            spectrogramImage.setPixelAt(x, y, juce::Colours::transparentBlack);
        }
    }

    for (auto x = 0; x < spectrogramImageDown.getWidth(); ++x)
    {
        for (auto y = 0; y < spectrogramImageDown.getHeight(); ++y)
        {
            //spectrogramImageDown.setPixelAt(x, y, juce::Colour::fromHSL(0, 1.0f, 0, 1.0f));
            spectrogramImageDown.setPixelAt(x, y, juce::Colours::transparentBlack);
        }
    }

    for (auto x = 0; x < spectrogramTimeImage.getWidth(); ++x)
    {
        for (auto y = 0; y < spectrogramTimeImage.getHeight(); ++y)
        {
            //spectrogramTimeImage.setPixelAt(x, y, juce::Colours::black);
            spectrogramTimeImage.setPixelAt(x, y, juce::Colours::transparentBlack);
        }

    }
    for (auto x = 0; x < spectrogramTimeImage.getWidth(); ++x)
    {
        spectrogramTimeImage.setPixelAt(x, spectrogramTimeImage.getHeight() / 2, juce::Colours::silver);
    }
    specgramSecCountReady = false;

    repaint();
}

void MainComponent::clearButtonClicked()
{
    changeState(Stopped);
    playButton.setImages(false, true, true, playButtonNormal, 1.0f, {}, playButtonNormal, 0.7f, {}, playButtonDown, 1.0f, {});
    playButton.setEnabled(false);
    stopButton.setEnabled(false);
    shutdownAudio();
    

    auto circBufferSize = fftSizeTSM * 10;

    outputCircularBuffer_S.clear();
    outputCircularBuffer_T.clear();
    outputCircularBuffer_N.clear();

    outputCircularBuffer_S.setSize(2, (int)circBufferSize);
    outputCircularBuffer_T.setSize(2, (int)circBufferSize);
    outputCircularBuffer_N.setSize(2, (int)circBufferSize);

    statusLabel.setText("No Audio File Loaded", juce::dontSendNotification);
    fileNameLable.setText(" ", juce::dontSendNotification);

    rmsMaster = -100.f;
    rmsSine = -100.f;
    rmsTrans = -100.f;
    rmsNoise = -100.f;
    
    resetpitchTSMButtonClicked();
    clearVisualStuff();
    //waveViewer.setVisible(false);
}




void MainComponent::changeState(TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
        case Stopped:                           
            playButton.setButtonText("Play");
            stopButton.setButtonText("Stop");
            //stopButton.setEnabled(false);
            frameIndex = 0;
            position = 0;
            break;

        case Starting:                          
            //playButton.setEnabled (false);
            //transportSource.start();
            setAudioChannels(0, numOutChannels);
            changeState(Playing);
            break;

        case Playing:                         
            statusLabel.setText("Playing", juce::dontSendNotification);

            playButton.setButtonText("Pause");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(true);
            backButton.setEnabled(true);
            break;

        case Pausing:
            setAudioChannels(0, 0);
            changeState(Paused);
            rmsMaster = -100.f;
            rmsSine = -100.f;
            rmsTrans = -100.f;
            rmsNoise = -100.f;
            break;

        case Paused:
            playButton.setButtonText("Resume");
            stopButton.setButtonText("Return to Start");
            statusLabel.setText("Paused", juce::dontSendNotification);
            backButton.setEnabled(true);
            break;

        case Stopping:                         
            setAudioChannels(0, 0);
            
            changeState(Stopped);
            rmsMaster = -100.f;
            rmsSine = -100.f;
            rmsTrans = -100.f;
            rmsNoise = -100.f;
            
            break;
        }
    }
}
bool MainComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent)
{
    if (key == juce::KeyPress::spaceKey)
    {
        playButtonClicked();
        return true;
    }

    return false;
}


void MainComponent::playButtonClicked()
{
    if ((state == Stopped) || (state == Paused))
    {
        changeState(Starting);
        playButton.setImages(false, true, true, pauseButtonNormal, 1.0f, {}, pauseButtonNormal, 0.7f, {}, pauseButtonDown, 1.0f, {});
    }
        
    else if (state == Playing)
    {
        changeState(Pausing);
        playButton.setImages(false, true, true, playButtonNormal, 1.0f, {}, playButtonNormal, 0.7f, {}, playButtonDown, 1.0f, {});
    }
        

}

void MainComponent::stopButtonClicked()
{
    playButton.setImages(false, true, true, playButtonNormal, 1.0f, {}, playButtonNormal, 0.7f, {}, playButtonDown, 1.0f, {});
    statusLabel.setText("Stopped", juce::dontSendNotification);
    if (state == Paused)
        changeState(Stopped);
    else
        changeState(Stopping);

}

void MainComponent::backButtonClicked()
{
    frameIndex = 0;
}


