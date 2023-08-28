#pragma once

#include <JuceHeader.h>
#include  <cmath>
#include <fftw3.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_gui_extra/juce_gui_extra.h>
//#include "meterComponent/horizontalMeter.h"
#include "meterComponent/verticalGradientMeter.h"
#include "meterComponent/OtherLookAndFeelSlider.h"
using namespace juce;
//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
using Vec2D = std::vector<std::vector<float>>;
using Vec2DComplex = std::vector<std::vector<std::complex<float>>>;
using Vec1D = std::vector<float>;
using Vec1DComplex = std::vector<std::complex<float>>;

enum TransportState
{
    Stopped,
    Starting,
    Playing,
    Pausing,
    Paused,
    Stopping
};

inline auto makeLowCutFilter(float systemFs)
{
    float cutoffFrequency = (systemFs) / 8.0f;

    //auto normalisedFrequency = cutoffFrequency / systemFs;
    //auto fp = normalisedFrequency/2;
    //float normalisedTransitionWidth  = 2*(normalisedFrequency - fp);

    // Define filter parameters
    float normalisedTransitionWidth = 0.1f;
    float passbandAmplitudedB = -0.1f;
    float stopbandAmplitudedB = -60.0f;


    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderChebyshev1Method(
        cutoffFrequency,
        systemFs,
        normalisedTransitionWidth,
        passbandAmplitudedB,
        stopbandAmplitudedB);
}


class MainComponent  : public juce::AudioAppComponent,
                       public juce::KeyListener,
                       public juce::Button::Listener,
                       public juce::Slider::Listener,
                       private juce::Timer

{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    static constexpr auto fftOrder1 = 13;                // 8192
    static constexpr auto fftSize1 = 1 << fftOrder1;     

    static constexpr auto fftOrder2 = 9;                 // 512
    static constexpr auto fftSize2 = 1 << fftOrder2;    

    // fft display component
    enum
    {
        fftOrder_visual_fft = 14,
        fftSize_visual_fft = 1 << fftOrder_visual_fft,
        scopeSize_visual_fft = 8192
    };


    void timerCallback();
    void sliderValueChanged(juce::Slider* slider) override;
    void pushNextSampleIntoFifo_fft(float sampleFFtAndSpecgram, float sampleSpecgramDown) noexcept;

    void drawNextFrameOfSpectrum();
    void drawNextLineOfSpectrogram();
    void drawNextLineOfSpectrogramDown();
    void drawFrame(juce::Graphics& g, int x, int y, int width, int height);
    bool specgramSecCountReady = false;
private:
    //==============================================================================
    // Your private member variables go here...
    
    void changeState(TransportState newState);
    void openButtonClicked();
    void clearButtonClicked();
    void saveButtonClicked();
    void playButtonClicked();
    void stopButtonClicked();
    void backButtonClicked();
    
    void buttonClicked(juce::Button* button);
    void clearVisualStuff();

    void matchTSMButtonClicked();
    void resetpitchTSMButtonClicked();
    

    juce::ImageButton openButton;
    juce::ImageButton clearButton;
    juce::ImageButton saveButton;

    juce::ImageButton playButton;
    juce::ImageButton stopButton;
    juce::ImageButton backButton;


    juce::Slider sineSlider;
    juce::Slider transientSlider;
    juce::Slider noiseSlider;

    juce::Slider progressBarSlider;
    float instantFrameRatio = 0;
    float instantFrameRatio_prev = 0;
    

    juce::ImageButton sineButton;
    juce::ImageButton transientButton;
    juce::ImageButton noiseButton;
    juce::ImageButton masterButton;

    // Store the button states
    bool sineButtonState;
    bool transientButtonState;
    bool noiseButtonState;
    bool masterButtonState;


    juce::ImageButton transientPreserveButton;
    juce::ImageButton phaseLockButton;

    bool transientPreserveState = true;
    bool phaseLockState = true;

    bool hiResState = false;


    juce::Slider masterSlider;
    juce::Label masterLabel;


    juce::Slider tsmSlider;
    juce::Label tsmLabel;

    juce::Slider resampleSlider;
    juce::Label resampleLabel;


    juce::ImageButton matchTSMButton;
    juce::ImageButton resetpitchTSMButton;
    bool resampleChanged = false;
    bool resampleReset = false;
    bool resampleMatch = false;

    juce::Label statusLabel;
    juce::Label fileNameLable;


    juce::Slider waveZoomTime;
    juce::Slider waveZoomAmp;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::AudioFormatManager formatManager;
    juce::AudioSampleBuffer fileBuffer;
    int position = 0;


    bool isPlaying;
    bool isReady = false;
    TransportState state;
    int numOutChannels = 2;


    int hopSize1 = fftSize1 / 8;
    int hopSize2 = fftSize2 / 8;

    
    juce::dsp::FFT forwardFFT1;
    juce::dsp::FFT inverseFFT1;

    juce::dsp::FFT forwardFFT2;
    juce::dsp::FFT inverseFFT2;

   

    std::vector<std::vector<std::complex<float>>> stftBuffer1_L, stftBuffer1_R;
    std::vector<std::vector<std::complex<float>>> stftBuffer2_L, stftBuffer2_R;

    std::vector<float> win_hann1[fftSize1]; 
    std::vector<float> win_hann2[fftSize2];

    Vec2DComplex XS, XT, XN; // stft Matrix for 3 components
    Vec1D RtVec, RnVec;

    Vec2D S_matrix1, T_matrix1, N_matrix1;
    Vec2D S_matrix2, T_matrix2, N_matrix2;

    juce::AudioSampleBuffer outputBuffer_S;
    juce::AudioSampleBuffer outputBuffer_T;
    juce::AudioSampleBuffer outputBuffer_N;

    //juce::AudioSampleBuffer outputBuffer_Y; // Undecomposed signal (original)

    Vec1D sineBuff;
    Vec1D transBuff;
    Vec1D NoiseBuff;

    
    void phaseVocoder(int channel, int circBufferSize, int outWritePtr, juce::AudioBuffer<float>& OcircBuffer,
        Vec1DComplex &currentFrameSTFT, int fftSize, int hopSize, Vec1D window, juce::dsp::FFT& inverseFFT);
        //Vec2DComplex& stftBuffer, int fftSize, int hopSize, int nFrames, Vec1D window, juce::dsp::FFT& inverseFFT);

    void transientVocoder(int channel, int circBufferSize, int outWritePtr, juce::AudioBuffer<float>& outputCircularBuffer,
        Vec1D& currentFrameTime, int fftSize, int hopSize, Vec1D window);


    void FuzzyPV_Sine(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi);
    //void FuzzyPV_Noise(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi, Vec1D& RnVec);
    void FuzzyPV_Noise(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi);
    //void FuzzyPV_Transient(Vec1DComplex& X, int hopSizeAna, int hopSizeSyn, float ola_coef, int frameIndex, Vec1D& phi0, Vec1D& psi, Vec1D& RtVec);

    int nWin_syn = 4096;
    int fftSizeTSM = nWin_syn;
    int fftOrderTSM = 12;
    juce::dsp::FFT forwardFFT_TSM;
    juce::dsp::FFT inverseFFT_TSM;

    Vec1D phi0_S, phi0_T, phi0_N;
    Vec1D psi_S, psi_T, psi_N;
    float Rt_0 = 0.f, Rt_d1=0.f, Rt_d2=0.f;
    int attack_detected = 0;
    int decay_detected = 0;

    std::vector<bool> decay_bins;
    std::vector<float> decay_gains;
    std::vector<bool> attack_bins;
    std::vector<float> attack_gains;
    std::vector<bool> bins_to_reset;
    int decay_length = 0;
    int reset_frame = 0;

    // PV variables
    Vec1D omega;


    int decSigLen, decSigLen_og; // outLen or the length of the originally decomposed signal
    Vec1D yS, yT, yT_tsm, yN; // decomposed signal vector
    Vec1D yS_og, yT_og, yT_tsm_og, yN_og; // originally decomposed signal vector
    Vec1D yT_tsm_resamp;
    int decSigLen0;

    std::vector<size_t> peakLocs, peakLocs_resamp;

    juce::AudioBuffer<float> outputCircularBuffer_S; //output circular buffer for Sine    
    juce::AudioBuffer<float> outputCircularBuffer_T; //output circular buffer for Transient   
    juce::AudioBuffer<float> outputCircularBuffer_N; //output circular buffer for Noise
    //int outWritePtr = hopSize1; //output buffer write position
    int outWritePtr = 0; //output buffer write position
    int outReadPtr = 0; //output buffer read position
    int hopCount = 0;
    int frameIndex = 0;
    int frameCount = 0;

    //float currentLevel = 0.0f, previousLevel = 0.0f;
    float sm_masterL, tar_masterL;
    float sm_SineL, tar_SineL;
    float sm_NoiseL, tar_NoiseL;
    float sm_TransientL, tar_TransientL;
    float tau = 0.9990;

    int hopMem = 1;
    int nHopA_prev = nWin_syn / 8; // init memory/state of analysis hopSizeA
    float resampleRate_prev = 1.0f;
    int nFrames_prev = 1;
    int nFramesNew = 1;
    bool resampleRequest = false;

    std::vector<float> win_hann_syn;

    int systemFs = 0;
    float fftFs = 48000;
    // ========= Visualization components 
    // time domain waveform
    juce::AudioVisualiserComponent waveViewer;
    juce::Image waveViewerGridBackground;

    // fft spectrum
    juce::dsp::FFT forwardFFT_visual_fft;
    //juce::dsp::FFT forwardFFT_visual_specgram, forwardFFT_visual_specgramDown;
    juce::dsp::WindowingFunction<float> window_visual_fft;
    //juce::dsp::WindowingFunction<float> window_visual_specgram, window_visual_specgramDown;

    float fifo_visual_fft[fftSize_visual_fft];
    float fifo_visual_specgram[fftSize_visual_fft];
    float fifo_visual_specgramDown[fftSize_visual_fft];

    float fftData_visual_fft[2 * fftSize_visual_fft];
    float fftData_visual_specgram[2 * fftSize_visual_fft];                // spectrum
    float fftData_visual_specgramDown[2 * fftSize_visual_fft];  // filled up with lowpass (antialias) signal    
    float fftData_visual_specgramDecimate[2 * fftSize_visual_fft];  // decimated and zeropadded version of fftData_visual_specgramDown 

    // smooth start with zeros
    float fftData_visual_fft_prev[2 * fftSize_visual_fft] = {};      
    float fftData_visual_specgram_prev[2 * fftSize_visual_fft] = {};


    //std::array<float, fftSize_visual * 2> fftData_visual2;   // spectrogram
    
    int fifoIndex_visual_fft = 0;
    int fifoIndex_visual_specgram = 0;// fftSize_visual_specgram / 8;
    int fifoIndex_visual_specgramDown = 0;

    
    bool nextFFTBlockReady_visual_fft = false;
    bool nextFFTBlockReady_visual_specgram = false;
    bool nextFFTBlockReady_visual_specgramDown = false;
    float scopeData_visual_fft[scopeSize_visual_fft];

    // specgram

    juce::Image spectrogramImage;
    juce::Image spectrogramImageDown;
    juce::Image spectrogramTimeImage;


    juce::Image fftGridBackground;

 
    //=========
    juce::Image iconLemon;
    juce::Image windowBackground;

    Vec1D mAudioPoints;
    //CustomImageButton playButtonTest;
    juce::LookAndFeel_V4 otherLookAndFeel;

    OtherLookAndFeelVertical otherLookAndFeelVertical;
    OtherLookAndFeelHorizontal otherLookAndFeelHorizontal;
    OtherLookAndFeelProgressBar otherLookAndFeelProgressBar;
    OtherLookAndFeelWaveZoom otherLookAndFeelWaveZoom;

    Rectangle<float> playerRect, mixerRect, tsmRect, resampRect, waveformRect, fftAnaRect, specgramRect;

    // for level meters
    float rmsMaster = -100.f;
    float rmsMaster_prev = 0.f;

    float rmsSine = -100.f;
    float rmsSine_prev = 0.f;

    float rmsTrans = -100.f;
    float rmsTrans_prev = 0.f;

    float rmsNoise = -100.f;
    float rmsNoise_prev = 0.f;

    bool isPaintWave = false;

    Gui::verticalGradientMeter verticalMeterMaster, verticalMeterSine, verticalMeterTrans, verticalMeterNoise;

    juce::ImageButton playPlauseButton;

    Image playButtonNormal = ImageCache::getFromMemory(BinaryData::playButtonNormal_png, BinaryData::playButtonNormal_pngSize);
    Image playButtonDown = ImageCache::getFromMemory(BinaryData::playButtonDown_png, BinaryData::playButtonDown_pngSize);

    Image pauseButtonNormal = ImageCache::getFromMemory(BinaryData::pauseButtonNormal_png, BinaryData::pauseButtonNormal_pngSize);
    Image pauseButtonDown = ImageCache::getFromMemory(BinaryData::pauseButtonDown_png, BinaryData::pauseButtonDown_pngSize);

    Image stopButtonNormal = ImageCache::getFromMemory(BinaryData::stopButtonNormal_png, BinaryData::stopButtonNormal_pngSize);
    Image stopButtonDown = ImageCache::getFromMemory(BinaryData::stopButtonDown_png, BinaryData::stopButtonDown_pngSize);

    Image backButtonNormal = ImageCache::getFromMemory(BinaryData::backButtonNormal_png, BinaryData::backButtonNormal_pngSize);
    Image backButtonDown = ImageCache::getFromMemory(BinaryData::backButtonDown_png, BinaryData::backButtonDown_pngSize);

    Image PlayerButtonCleanBg = ImageCache::getFromMemory(BinaryData::PlayerButtonCleanBg_png, BinaryData::PlayerButtonCleanBg_pngSize);

    Image openButtonNormal = ImageCache::getFromMemory(BinaryData::openButtonNormal_png, BinaryData::openButtonNormal_pngSize);
    Image openButtonDown = ImageCache::getFromMemory(BinaryData::openButtonDown_png, BinaryData::openButtonDown_pngSize);

    Image clearButtonNormal = ImageCache::getFromMemory(BinaryData::clearButtonNormal_png, BinaryData::clearButtonNormal_pngSize);
    Image clearButtonDown = ImageCache::getFromMemory(BinaryData::clearButtonDown_png, BinaryData::clearButtonDown_pngSize);
    Image openClearButtonCleanBg = ImageCache::getFromMemory(BinaryData::openClearButtonCleanBg_png, BinaryData::openClearButtonCleanBg_pngSize);


    Image sineButtonOn = ImageCache::getFromMemory(BinaryData::sineButtonOn_png, BinaryData::sineButtonOn_pngSize);
    Image sineButtonOff = ImageCache::getFromMemory(BinaryData::sineButtonOff_png, BinaryData::sineButtonOff_pngSize);
    Image transButtonOn = ImageCache::getFromMemory(BinaryData::transButtonOn_png, BinaryData::transButtonOn_pngSize);
    Image transButtonOff = ImageCache::getFromMemory(BinaryData::transButtonOff_png, BinaryData::transButtonOff_pngSize);
    Image noiseButtonOn = ImageCache::getFromMemory(BinaryData::noiseButtonOn_png, BinaryData::noiseButtonOn_pngSize);
    Image noiseButtonOff = ImageCache::getFromMemory(BinaryData::noiseButtonOff_png, BinaryData::noiseButtonOff_pngSize);
    Image masterButtonOn = ImageCache::getFromMemory(BinaryData::masterButtonOn_png, BinaryData::masterButtonOn_pngSize);
    Image masterButtonOff = ImageCache::getFromMemory(BinaryData::masterButtonOff_png, BinaryData::masterButtonOff_pngSize);

    Image transPreserveButtonActive = ImageCache::getFromMemory(BinaryData::transPreserveButtonActive_png, BinaryData::transPreserveButtonActive_pngSize);
    Image transPreserveButtonDown = ImageCache::getFromMemory(BinaryData::transPreserveButtonDown_png, BinaryData::transPreserveButtonDown_pngSize);
    Image transPreserveButtonNormal = ImageCache::getFromMemory(BinaryData::transPreserveButtonNormal_png, BinaryData::transPreserveButtonNormal_pngSize);

    Image pvButtonActive = ImageCache::getFromMemory(BinaryData::pvButtonActive_png, BinaryData::pvButtonActive_pngSize);
    Image pvButtonDown = ImageCache::getFromMemory(BinaryData::pvButtonDown_png, BinaryData::pvButtonDown_pngSize);
    Image pvButtonNormal = ImageCache::getFromMemory(BinaryData::pvButtonNormal_png, BinaryData::pvButtonNormal_pngSize);


    Image resetButtonNormal = ImageCache::getFromMemory(BinaryData::resetButtonNormal_png, BinaryData::resetButtonNormal_pngSize);
    Image resetButtonDown = ImageCache::getFromMemory(BinaryData::resetButtonDown_png, BinaryData::resetButtonDown_pngSize);

    Image matchButtonNormal = ImageCache::getFromMemory(BinaryData::matchButtonNormal_png, BinaryData::matchButtonNormal_pngSize);
    Image matchButtonDown = ImageCache::getFromMemory(BinaryData::matchButtonDown_png, BinaryData::matchButtonDown_pngSize);

    Image hiresButtonActive = ImageCache::getFromMemory(BinaryData::hiresButtonActive_png, BinaryData::hiresButtonActive_pngSize);
    Image hiresButtonNormal = ImageCache::getFromMemory(BinaryData::hiresButtonNormal_png, BinaryData::hiresButtonNormal_pngSize);
    
    int sampleCount = 0;
    int secCount = 0;

    ColourGradient bgGradient{};
    juce::Colour scopeColor{ juce::Colour::fromRGB(135, 177, 145) };


    juce::ImageButton hiResButton;

    //juce::dsp::IIR::Coefficients<float>::Ptr lowpassFilterCoefficients;
    juce::ReferenceCountedArray<juce::dsp::IIR::Coefficients<float>> lowpassFilterCoeff;
    juce::dsp::IIR::Filter<float> lowpassFilter;
    juce::dsp::ProcessSpec processSpec;


    void updateAntialiasFilter();


    juce::File fileToSave;
    std::unique_ptr<juce::FileOutputStream> fileOutputStream;
    int samplesToRecord; // Number of samples to record for saving
    int samplesRecorded; // Number of samples recorded so far
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)



};

