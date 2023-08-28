/*
  ==============================================================================

    STN_utils.h
    Created: 3 Jul 2023 3:18:02pm
    Author:  Tantep

  ==============================================================================
*/

#pragma once

#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include <juce_dsp/juce_dsp.h>
#include <JuceHeader.h>
#include <random>
#include <numeric>
#include <unordered_map>
#include <set>
#include "avx2sort.h"

using Vec2D = std::vector<std::vector<float>>;
using Vec2DComplex = std::vector<std::vector<std::complex<float>>>;
using Vec1D = std::vector<float>;
#define j_Pi     juce::MathConstants<float>::pi

inline float roundToDecimal(float value, int decimalPlaces)
{
    float factor = std::pow(10.0f, decimalPlaces);
    return std::round(value * factor) / factor;
}

// Function to print the real and imaginary parts of a complex number
void printComplex(const std::complex<float>& value) {
    DBG(value.real() << " + " << value.imag() << "i");
    // usage ex. printComplex(Ynew[row][col]);
}

void naive_decimate(std::vector<float>& input, double decimationFactor) {
    std::vector<float> decimated;


    // Calculate the number of output samples after decimation
    int numOutputSamples = static_cast<int>(std::round(input.size() / decimationFactor));
    decimated.resize(numOutputSamples);

    // Decimate the input vector using linear interpolation
    for (int i = 0; i < numOutputSamples; ++i) {
        double index = i * decimationFactor;
        int leftIndex = static_cast<int>(std::floor(index));
        int rightIndex = static_cast<int>(std::ceil(index));

        double frac = index - leftIndex;
        decimated[i] = (1.0 - frac) * input[leftIndex] + frac * input[rightIndex];
    }
    input.resize(numOutputSamples);
    input = decimated;
}


inline Vec1D linearResampleVec(const Vec1D& originalSignal, float ratio)
{
    Vec1D newSignal; // resampled signal
    //float ratio = static_cast<float>(newFs) / originalFs;

    int originalSize = static_cast<int>(originalSignal.size());
    int newSize = static_cast<int>(std::floor(ratio * originalSize));

    const auto lerp = [](float v0, float v1, float t)
    {
        return (1.f - t) * v0 + t * v1;
    };

    const float scale = static_cast<float>(originalSize) / newSize;
    float index = 0.f;

    for (int i = 0; i < newSize; ++i)
    {
        const auto wholeIndex = static_cast<int>(std::floor(index));
        const auto fractionIndex = index - wholeIndex;
        const auto sampleA = originalSignal[wholeIndex];
        const auto sampleB = originalSignal[wholeIndex + 1];
        newSignal.push_back(lerp(sampleA, sampleB, fractionIndex));
        index += scale;
    }

    return newSignal;
}



inline Vec1D linearResample(const float* originalSignal,int originalSize, int originalFs, int newFs)
{
    Vec1D newSignal; // resampled signal
    float ratio = static_cast<float>(newFs) / originalFs;

    
    int newSize = std::floor(ratio * originalSize);

    const auto lerp = [](float v0, float v1, float t)
    {
        return (1.f - t) * v0 + t * v1;
    };

    const float scale = static_cast<float>(originalSize) / newSize;
    float index = 0.f;

    //for (int i = 0; i < newSize; ++i)
    //{
    //    int wholeIndex = static_cast<int>(index);
    //    float fractionIndex = index - wholeIndex;

    //    // Bounds checking for wholeIndex
    //    wholeIndex = std::max(0, std::min(wholeIndex, originalSize - 1));

    //    // Bounds checking for wholeIndex + 1
    //    int nextIndex = std::max(0, std::min(wholeIndex + 1, originalSize - 1));

    //    float sampleA = originalSignal[wholeIndex];
    //    float sampleB = originalSignal[nextIndex];

    //    newSignal.push_back(lerp(sampleA, sampleB, fractionIndex));
    //    index += scale;
    //}

    for (int i = 0; i < newSize; ++i)
    {
        const auto wholeIndex = (int)floor(index);
        const auto fractionIndex = index - wholeIndex;
        const auto sampleA = originalSignal[wholeIndex];
        const auto sampleB = originalSignal[wholeIndex + 1];
        newSignal.push_back( lerp(sampleA, sampleB, fractionIndex));
        index += scale;
    }

    return newSignal;
}

//inline float antiAliasingLookup(float Fs)
//{
//    // returns coefficients for 2nd order Chebyshev type I filter, cutoff at 3kHz, for different Fs
//    float coeff[6] = { };       // all elements are 0
//}
inline void normalizeSignal(std::vector<float>& x)
{

    std::vector<float> abs_x = x;
    for (float& sample : abs_x) {
        sample = std::abs(sample);
    }

    float maxAbs = *max_element(abs_x.begin(), abs_x.end());

    
    //for (float& sample : x) {
    //    sample = 0.7071 * (sample / maxAbs); 
    //}
    if (maxAbs > 0.0f) {
        for (float& sample : x) {
            sample = 0.95 * (sample / maxAbs);
        }
    }
}

inline Vec2D medfiltVerticalAVX2(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianV)
{
    const int numChannels = static_cast<int>(abs_stftBuffer.size());
    const int numSamples = static_cast<int>(abs_stftBuffer[0].size());
    Vec2D X_V_median(numChannels, std::vector<float>(numSamples, 0.0f));

    const int pad = nMedianV / 2;
    std::vector<int> med_vec((int)numSamples + 2 * pad, 0);
    std::vector<int> kernel_vec(nMedianV, 0);

    for (int i = 0; i < numChannels; ++i)
    {
        // Copy the entire row to med_vec for faster indexing
        for (int h = 0; h < numSamples; ++h)
        {
            med_vec[h + pad] = abs_stftBuffer[i][h] * 100000;
        }

        for (int h = 0; h < numSamples; ++h)
        {
            // Extract the kernel directly from med_vec using indexing
            for (int k = 0; k < nMedianV; ++k)
            {
                kernel_vec[k] = med_vec[h + k];
            }

            // Find the median element using nth_element
            //std::nth_element(kernel_vec.begin(), kernel_vec.begin() + pad, kernel_vec.end());
            avx2::quickselect(kernel_vec.data(), nMedianV, pad);
            X_V_median[i][h] = kernel_vec[pad] / 100000.f;
        }
    }

    return X_V_median;
}

inline Vec2D medfiltHorizontalAVX2(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianH)
{
    const int numChannels = static_cast<int>(abs_stftBuffer.size());
    const int numSamples = static_cast<int>(abs_stftBuffer[0].size());
    Vec2D X_H_median(numChannels, std::vector<float>(numSamples, 0.0f));

    const int pad = nMedianH / 2;
    std::vector<int> med_vec((int)numChannels + 2 * pad, 0);
    std::vector<int> kernel_vec(nMedianH, 0);

    for (int h = 0; h < numSamples; ++h)
    {
        // Copy the entire column to med_vec for faster indexing
        for (int i = 0; i < numChannels; ++i)
        {
            med_vec[i + pad] = abs_stftBuffer[i][h]* 100000;
        }

        for (int i = 0; i < numChannels; ++i)
        {
            // Extract the kernel directly from med_vec using indexing
            for (int k = 0; k < nMedianH; ++k)
            {
                kernel_vec[k] = med_vec[i + k];
            }

            // Find the median element using avx2::quickselect
            avx2::quickselect(kernel_vec.data(), nMedianH, pad);
            X_H_median[i][h] = kernel_vec[pad] / 100000.f;
        }
    }

    return X_H_median;
}

inline Vec2D medfiltVertical(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianV)
{
    const int numChannels = static_cast<int>(abs_stftBuffer.size());
    const int numSamples = static_cast<int>(abs_stftBuffer[0].size());
    Vec2D X_V_median(numChannels, std::vector<float>(numSamples, 0.0f));

    const int pad = nMedianV / 2;
    std::vector<float> med_vec(numSamples + 2 * pad, 0.0f);
    std::vector<float> kernel_vec(nMedianV, 0.0f);

    for (int i = 0; i < numChannels; ++i)
    {
        //// Copy the entire row to med_vec for faster indexing
        //for (int h = 0; h < numSamples; ++h)
        //{
        //    med_vec[h + pad] = abs_stftBuffer[i][h];
        //}

        for (int h = 0; h < numSamples; ++h)
        {
            med_vec[h + pad] = abs_stftBuffer[i][h];
            // Extract the kernel directly from med_vec using indexing
            for (int k = 0; k < nMedianV; ++k)
            {
                kernel_vec[k] = med_vec[h + k];
            }

            // Find the median element using nth_element
            std::nth_element(kernel_vec.begin(), kernel_vec.begin() + pad, kernel_vec.end());
            X_V_median[i][h] = kernel_vec[pad];
        }
    }

    return X_V_median;
}
inline Vec2D medfiltHorizontal(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianH)
{
    const int numChannels = static_cast<int>(abs_stftBuffer.size());
    const int numSamples = static_cast<int>(abs_stftBuffer[0].size());
    Vec2D X_H_median(numChannels, std::vector<float>(numSamples, 0.0f));

    const int pad = nMedianH / 2;
    std::vector<float> med_vec(numChannels + 2 * pad, 0.0f);
    std::vector<float> kernel_vec(nMedianH, 0.0f);

    for (int h = 0; h < numSamples; ++h)
    {
        //// Copy the entire column to med_vec for faster indexing
        //for (int i = 0; i < numChannels; ++i)
        //{
        //    med_vec[i + pad] = abs_stftBuffer[i][h];
        //}

        for (int i = 0; i < numChannels; ++i)
        {
            med_vec[i + pad] = abs_stftBuffer[i][h];
            // Extract the kernel directly from med_vec using indexing
            for (int k = 0; k < nMedianH; ++k)
            {
                kernel_vec[k] = med_vec[i + k];
            }

            // Find the median element using nth_element
            std::nth_element(kernel_vec.begin(), kernel_vec.begin() + pad, kernel_vec.end());
            X_H_median[i][h] = kernel_vec[pad];
        }
    }

    return X_H_median;
}

//inline Vec2D medfiltVertical(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianV)
//{
//    Vec2D X_V_median(abs_stftBuffer.size(), std::vector<float>(abs_stftBuffer[0].size(), 0.0f));
//
//
//    auto pad = std::floor(nMedianV / 2);
//    std::vector<float> med_vec(abs_stftBuffer[0].size() + 2 * pad, 0.0f);
//    std::vector<float> kernel_vec(nMedianV, 0.0f);
//
//    for (int i = 0; i < (int)abs_stftBuffer.size(); ++i)
//    {
//        for (int h = 0; h < (int)abs_stftBuffer[i].size(); ++h)
//        {
//            med_vec[h + pad] = abs_stftBuffer[i][h];
//        }
//
//        for (int h = 0; h < (int)abs_stftBuffer[i].size(); ++h)
//        {
//            std::copy(med_vec.begin() + h, med_vec.begin() + h + nMedianV, kernel_vec.begin());
//            std::nth_element(kernel_vec.begin(), kernel_vec.begin() + pad, kernel_vec.end());
//            X_V_median[i][h] = kernel_vec[pad];
//        }
//
//    }
//    return X_V_median;
//}
//
//
//inline Vec2D medfiltHorizontal(const std::vector<std::vector<float>>& abs_stftBuffer, int nMedianH)
//{
//    Vec2D X_H_median(abs_stftBuffer.size(), std::vector<float>(abs_stftBuffer[0].size(), 0.0f));
//
//    const int pad = std::floor(nMedianH / 2);
//    std::vector<float> med_vec(abs_stftBuffer.size() + 2 * pad, 0.0f);
//    std::vector<float> kernel_vec(nMedianH, 0.0f);
//
//    for (int h = 0; h < (int)abs_stftBuffer[0].size(); ++h)
//    {
//        for (int i = 0; i < (int)abs_stftBuffer.size(); ++i)
//        {
//            med_vec[i + pad] = abs_stftBuffer[i][h];
//        }
//
//        for (int i = 0; i < (int)abs_stftBuffer.size(); ++i)
//        {
//            std::copy(med_vec.begin() + i, med_vec.begin() + i + nMedianH, kernel_vec.begin());
//            std::nth_element(kernel_vec.begin(), kernel_vec.begin() + pad, kernel_vec.end());
//            X_H_median[i][h] = kernel_vec[pad];
//
//        }
//
//    }
//    return X_H_median;
//}

inline Vec2D transientness(const Vec2D& X_H_median, Vec2D& X_V_median)
{
    size_t numRows = X_V_median.size();
    size_t numCols = X_V_median[0].size();
    // Perform element-wise division
    for (size_t row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < numCols; ++col) {
            float denominator = X_V_median[row][col] + X_H_median[row][col];
            if (denominator != 0.0f) {
                X_V_median[row][col] /= denominator;
            }
            else {
                X_V_median[row][col] = 0.0f;  // Handle division by zero
            }

            if (std::isnan(X_V_median[row][col])) {
                X_V_median[row][col] = 0.0f;  // Handle NaN values
            }
        }
    }
    return X_V_median;
}

inline std::tuple<Vec2D, Vec2D, Vec2D> decSTN(const std::vector<std::vector<float>>& Rt, float G2, float G1)
{
    size_t numRows = Rt.size();
    size_t numCols = (numRows > 0) ? Rt[0].size() : 0;
    size_t newNumCols = 2 * (numCols - 1); // New number of columns after appending the conjugate and flipped part

    std::vector<std::vector<float>> S(numRows, std::vector<float>(newNumCols));
    std::vector<std::vector<float>> T(numRows, std::vector<float>(newNumCols));
    std::vector<std::vector<float>> N(numRows, std::vector<float>(newNumCols));


    float Rs, Rs_minus_G2, Rt_minus_G2;

    for (size_t row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < numCols; ++col) {
            Rs = 1 - Rt[row][col];
            Rs_minus_G2 = Rs - G2;
            Rt_minus_G2 = Rt[row][col] - G2;

            if (Rs >= G1) {
                S[row][col] = 1.0f;
            }
            else if (Rs < G2) {
                S[row][col] = 0.0f;
            }
            else {
                S[row][col] = std::sin(j_Pi * Rs_minus_G2 / (2.0f * (G1 - G2)));
                S[row][col] = S[row][col] * S[row][col];
            }

            if (Rt[row][col] >= G1) {
                T[row][col] = 1.0f;
            }
            else if (Rt[row][col] < G2) {
                T[row][col] = 0.0f;
            }
            else {
                T[row][col] = std::sin(j_Pi * Rt_minus_G2 / (2.0f * (G1 - G2)));
                T[row][col] = T[row][col] * T[row][col];
            }

            N[row][col] = 1.0f - S[row][col] - T[row][col];
        }
        // Append the conjugate and flipped part to each row of S matrix
        for (size_t col = numCols; col < newNumCols; ++col)
        {
            size_t flippedCol = newNumCols - col;
            S[row][col] = S[row][flippedCol];
            T[row][col] = T[row][flippedCol];
            N[row][col] = N[row][flippedCol];
        }
    }

    // Return S, T, and N as a tuple
    return std::make_tuple(S, T, N);
}

inline Vec2DComplex applyMask(const Vec2D& maskMatrix, const Vec2DComplex& stftBuffer) {
    // Perfrom elementwise multiplication between 2D vector (STN) and 2D complex vector (stftBuffer)
    size_t numRows = maskMatrix.size();
    size_t numCols = maskMatrix[0].size();

    Vec2DComplex result(numRows, std::vector<std::complex<float>>(numCols));

    for (size_t row = 0; row < numRows; ++row) {
        for (size_t col = 0; col < numCols; ++col) {
            std::complex<float> value = maskMatrix[row][col] * stftBuffer[row][col];
            result[row][col] = value;
        }
    }

    return result;
}


//std::vector<float> zeroPadTime(std::vector<float> inputSignal, size_t nWin)
//{
//
//    // Step 1: Add zeros at the beginning
//    std::vector<float> paddedSignal(inputSignal.begin(), inputSignal.end());
//    paddedSignal.insert(paddedSignal.begin(), nWin / 4, 0.0f);
//
//    // Step 2: Add zeros at the end
//    // Calculate the number of samples needed to make the signal length a multiple of the window size
//    size_t signalLength = paddedSignal.size();
//    size_t pad = (nWin - (signalLength % nWin)) % nWin;
//    paddedSignal.insert(paddedSignal.end(), pad, 0.0f);
//
//    return paddedSignal;
//}

std::vector<float> zeroPadTime(Vec1D inputSignal, size_t nWin)
{
    // Calculate the fade in and fade out length
    size_t fadeLength = nWin/4;

    // Step 1: Apply fade in
    for (size_t i = 0; i < fadeLength; ++i)
    {
        float fadeValue = static_cast<float>(i) / static_cast<float>(fadeLength);
        inputSignal[i] *= fadeValue;
    }

    // Step 2: Apply fade out
    size_t signalLength = inputSignal.size();
    for (size_t i = signalLength - fadeLength; i < signalLength; ++i)
    {
        float fadeValue = static_cast<float>(signalLength - i - 1) / static_cast<float>(fadeLength);
        inputSignal[i] *= fadeValue;
    }

    // Step 3: Add zeros at the beginning
    std::vector<float> paddedSignal(inputSignal.begin(), inputSignal.end());
    paddedSignal.insert(paddedSignal.begin(), nWin / 4, 0.0f);

    // Step 4: Add zeros at the end
    // Calculate the number of samples needed to make the signal length a multiple of the window size
    size_t pad = (nWin - (paddedSignal.size() % nWin)) % nWin;
    paddedSignal.insert(paddedSignal.end(), pad, 0.0f);

    return paddedSignal;
}

inline Vec2DComplex doForwardSTFT(Vec2DComplex& stftBuffer, int fftSize, int hopSize, std::vector<float> window, std::vector<float> inputSignal, juce::dsp::FFT& forwardFFT,
    int nFrames, int nBins)
{
    auto winSize = fftSize;
    //stftBuffer.clear();
    stftBuffer.resize(nFrames, Vec1DComplex(nBins));
    int bufferIndex = 0; // Index for accessing stftBuffer

    for (int i = 0; i + winSize <= inputSignal.size(); i += hopSize)
    {
        std::vector<std::complex<float>> grainL(winSize);
        std::vector<std::complex<float>> tempGrainL(winSize);

        for (int j = 0; j < winSize; ++j)
        {
            grainL[j] = inputSignal[i + j] * window[j];
        }

        forwardFFT.perform(grainL.data(), tempGrainL.data(), false);

        stftBuffer[bufferIndex] = tempGrainL;
        bufferIndex++;
    }
    return stftBuffer;
}
inline Vec1D makeHannWindow(int winSize)
{
    Vec1D win_hann(winSize);
    for (int i = 0; i < (int)winSize; ++i)
    {
        win_hann[i] = 0.5f - 0.5f * std::cos(2.0f * j_Pi * (float)i / (float(winSize - 1)));
    }
    return win_hann;
}

inline Vec2D makeAbsSTFT(Vec2DComplex& stftBuffer, const int numCols, const int numRows)
{
    std::vector<std::vector<float>> abs_stftBuffer(numCols, std::vector<float>(numRows, 0.0f)); // This is transpoed to be the same convention as stftBuffer
    //------------------ making absolute stftBuffer with nBins = fftSize/2+1 ------------------//
    for (size_t col = 0; col < numCols; ++col)
    {
        for (size_t row = 0; row < numRows; ++row)
        {
            abs_stftBuffer[col][row] = std::abs(stftBuffer[col][row]);
        }
    }
    return abs_stftBuffer;
}

float ola_norm_coef(const std::vector<float>& win_analysis, int nHop_synthesis)
{
    int nWin = win_analysis.size();
    int nHop = nHop_synthesis;

    std::vector<float> win(nWin);
    for (int i = 0; i < nWin; ++i) {
        win[i] = std::pow(win_analysis[i], 2);
    }

    int idx = nWin / 2 + 1;
    float y = win[idx];

    int m = 1;
    int i = idx - m * nHop;
    int j = idx + m * nHop;

    while (i > 0 && j <= nWin) {
        y += win[i] + win[j];
        m++;
        i = idx - m * nHop;
        j = idx + m * nHop;
    }

    return y;
}


inline std::vector<float> doInverseSTFT(Vec2DComplex& stftBuffer, int fftSize, int hopSize, int nFrames, std::vector<float> window, juce::dsp::FFT& inverseFFT)
{
    auto outLen = (nFrames - 1) * hopSize + fftSize;

    std::vector<float> y(outLen);//, 0.0f);

    std::vector<std::complex<float>> grainOut(fftSize);
    std::vector<std::complex<float>> tempGrainOut(fftSize);
    auto olaCoeff = ola_norm_coef(window, hopSize);
    int p = 0;
    for (int i = 0; i < (int)nFrames; ++i)
    {

        tempGrainOut = stftBuffer[i]; // Access the i-th frame from stftBuffer1_L

        inverseFFT.perform(tempGrainOut.data(), grainOut.data(), true);

        std::vector<float> grain(fftSize);

        for (int j = 0; j < (int)fftSize; ++j)
        {
            grain[j] = grainOut[j].real() * window[j] / 3;
            y[p + j] += grain[j];
        }

        p += hopSize;
    }

    return y;
}


inline float princArg(const float phase)
{
    return fmod(phase + j_Pi, -2.0f * j_Pi) + j_Pi;
}


inline Vec1D reconstructOLA(Vec2DComplex Rt_Rn, int fftSize, int hopSize, int nFrames, std::vector<float> window)
{
    auto outLen = (nFrames - 1) * hopSize + fftSize;

    Vec1D y(outLen, 0.0f);

    for (int i = 0, p = 0; i < nFrames; ++i, p += hopSize)
    {
        const Vec1DComplex& grainOut = Rt_Rn[i];

        for (int j = 0; j < fftSize; ++j)
        {
            y[p + j] += grainOut[j].real() * window[j] / 3;
        }
    }

    return y;
}


inline Vec2DComplex makeOverlapMatrix(Vec2DComplex & X_OverlapMat, int fftSize, int hopSize, Vec1D window, Vec1D inputSignal, int nFrames, int nBins)
{
    auto winSize = fftSize;
    X_OverlapMat.resize(nFrames, Vec1DComplex(fftSize));
    int bufferIndex = 0; // Index for accessing stftBuffer

    for (int i = 0; i + winSize <= inputSignal.size(); i += hopSize)
    {
        Vec1DComplex grain(winSize);

        for (int j = 0; j < winSize; ++j)
        {
            grain[j] = inputSignal[i + j] * window[j];
        }


        X_OverlapMat[bufferIndex] = grain;
        bufferIndex++;
    }
    return X_OverlapMat;
}

inline Vec2D makeOverlapMatrix_Real(Vec2D& X_OverlapMat, int fftSize, int hopSize, Vec1D window, Vec1D inputSignal, int nFrames, int nBins)
{
    auto winSize = fftSize;
    X_OverlapMat.resize(nFrames, Vec1D(fftSize));
    int bufferIndex = 0; // Index for accessing stftBuffer

    for (int i = 0; i + winSize <= inputSignal.size(); i += hopSize)
    {
        Vec1D grain(winSize);

        for (int j = 0; j < winSize; ++j)
        {
            grain[j] = inputSignal[i + j] * window[j];
        }


        X_OverlapMat[bufferIndex] = grain;
        bufferIndex++;
    }
    return X_OverlapMat;
}


inline Vec1DComplex getFrameFromInputSignal(int frameIndex, int fftSize, int hopSize, const Vec1D& window, const Vec1D& inputSignal)
{
    int winSize = fftSize;
    int startIndex = frameIndex * hopSize;

    Vec1DComplex grain(winSize);

    /*for (int j = 0; j < winSize; ++j)
    {
        grain[j] = inputSignal[startIndex + j] * window[j];
    }*/

    for (int j = 0; j < winSize; ++j)
    {
        int index = startIndex + j;
        if (index < inputSignal.size())
        {
            grain[j] = inputSignal[index] * window[j];
        }
        else
        {
            grain[j] = 0.0f; // Zero-padding for out-of-range samples
        }
    }

    return grain;
}

inline Vec1D getFrameFromInputSignal_Transient(int frameIndex, int fftSize, int hopSize, const Vec1D& window, const Vec1D& inputSignal)
{
    int winSize = fftSize;
    int startIndex = frameIndex * hopSize;

    Vec1D grain(winSize);

    for (int j = 0; j < winSize; ++j)
    {
        grain[j] = inputSignal[startIndex + j] * window[j];
    }

    return grain;
}

inline Vec1D getFrameFromInputSignal_RtRn(int frameIndex, int fftSize, int hopSize, const Vec1D& window, const Vec1D& inputSignal)
{
    int winSize = fftSize;
    int startIndex = frameIndex * hopSize;

    Vec1D grain(winSize);

    /*for (int j = 0; j < winSize; ++j)
    {
        grain[j] = inputSignal[startIndex + j] * window[j];
    }*/

    for (int j = 0; j < winSize; ++j)
    {
        int index = startIndex + j;
        if (index < inputSignal.size())
        {
            grain[j] = inputSignal[index] * window[j];
        }
        else
        {
            grain[j] = 0.0f; // Zero-padding for out-of-range samples
        }
    }

    return grain;
}

inline void transientReposition(Vec1D& yT_tsm, const Vec1D& yT, std::vector<size_t> peakLocs, float alpha,
                                int nFrames, int nHopS, int NFFT)//, int tsmSigLen)
{
    // Length after stretch
    int tsmSigLen = (nFrames - 1) * (nHopS) + NFFT; // (nCols_TSM - 1) * (nHopS) + nWin_syn;
    // Initialize yT_tsm
    yT_tsm.clear();
    yT_tsm.resize(tsmSigLen + NFFT + nHopS);
    

    //peakLocs.push_back(yT.size()); // append the length of the signal to account for the last transient

    // Process each peak/transient
    for (size_t i = 1; i < peakLocs.size(); ++i)
    {
        size_t framesize = std::min(peakLocs[i] - peakLocs[i - 1], static_cast<size_t>(8192));

        size_t oldstart = peakLocs[i - 1] - static_cast<size_t>(std::round(0.4 * framesize));
        if (oldstart < 0)
        {
            framesize += oldstart;
            oldstart = 0;
        }
        size_t oldstop = oldstart + framesize;
        if (oldstop > yT.size())
        {
            oldstop = yT.size();
            framesize = oldstop - oldstart;
        }

        size_t newstart = static_cast<size_t>(std::floor(alpha * peakLocs[i - 1])) - (peakLocs[i - 1] - oldstart);
        size_t newstop = newstart + framesize;

        for (size_t j = newstart; j <= newstop; ++j)
        {
            yT_tsm[j] = yT[oldstart + (j - newstart)];
        }
    }

    // Process the last peak/transient
    size_t last_framesize = yT.size() - peakLocs.back();
    size_t last_oldstart = peakLocs.back() - static_cast<size_t>(std::round(0.4 * last_framesize));
    if (last_oldstart < 0)
    {
        last_framesize += last_oldstart;
        last_oldstart = 0;
    }
    size_t last_oldstop = last_oldstart + last_framesize;
    size_t last_newstart = static_cast<size_t>(std::floor(alpha * peakLocs.back())) - (peakLocs.back() - last_oldstart);
    size_t last_newstop = last_newstart + last_framesize;

    for (size_t j = last_newstart; j <= last_newstop; ++j)
    {
        yT_tsm[j] = yT[last_oldstart + (j - last_newstart)];
    }
    
    //yT_tsm.resize(tsmSigLen);
    yT_tsm.resize(tsmSigLen + NFFT + nHopS);

}

inline std::tuple<Vec2D, Vec1D, Vec2D, Vec2D, Vec2D, Vec1D> findSpectralPeaks(Vec2DComplex X)
{
    // nPeaks = 1 x nFrames, 
    // psi = 1 x nBins,
    // other matrix are of size nBins x nFrames

    size_t nFrames = X.size();
    size_t NFFT    = X[0].size();
    size_t nBins   = (NFFT / 2) + 1;

    //Vec2D r(nFrames, Vec1D(nBins));
    Vec2D r(nFrames, Vec1D(nBins));
    Vec2D phi(nFrames, Vec1D(nBins));

    Vec2D r_synth(nFrames, Vec1D(nBins, 0.0f));

    Vec2D peak_loc(nFrames, Vec1D(nBins, 0.0f));
    Vec1D nPeaks(nFrames, 0.0f);
    Vec1D psi(nBins, 0.0f);

    // Using up to nBins (half of X)
    for (size_t col = 0; col < nFrames; ++col) 
    {
        for (size_t row = 0; row < nBins; ++row) 
        {
            r[col][row] = std::abs(X[col][row]);
            phi[col][row] = std::arg(X[col][row]);
        }
    }

    // Find spectral peaks
    for (size_t n = 0; n < nFrames; ++n) 
    {
        size_t i = 2;
        while (i < nBins - 2) {
            if (r[n][i] > r[n][i - 1] && r[n][i] > r[n][i - 2] && r[n][i] > r[n][i + 1] && r[n][i] > r[n][i + 2]) 
            {
                nPeaks[n] ++;
                peak_loc[n][nPeaks[n]-1] = i;
                i += 3;
            }
            else 
            {
                i++;
            }
        }
    }
    
    return std::make_tuple(peak_loc, nPeaks, r, r_synth, phi, psi);

}

//         RE-CHECK THIS FUNCTION !!!!!!!!!
//
inline std::tuple<Vec2D, Vec2D, Vec2D>  instantFreq_binAround(Vec2D peak_loc, Vec1D nPeaks, Vec2D phi, float nHopA)
{
    //inst_freq = nBins  x nFrames
    //bin_low =   max(nPeaks) x nFrames
    //bin_high =  max(nPeaks) x nFrames);

    size_t nFrames = phi.size();
    size_t nBins = phi[0].size();
    size_t NFFT = (nBins - 1) * 2;

    // Find the maximum nPeaks
    auto max_nPeaks_iter = std::max_element(nPeaks.begin(), nPeaks.end());
    float max_nPeaks = *max_nPeaks_iter;

    Vec2D inst_freq(nFrames, Vec1D(nBins, 0.0f));

    Vec2D bin_low(nFrames, Vec1D(max_nPeaks, 0.0f));
    Vec2D bin_high(nFrames, Vec1D(max_nPeaks, 0.0f));

    // PV variables
    std::vector<float> omega(nBins);
    for (int k = 0; k < nBins; ++k)
    {
        omega[k] = 2 * j_Pi * k / NFFT;
    }

    for (size_t n = 1; n < nFrames; ++n) // starting at second frame
    {
        if (nPeaks[n] > 0) // Phase locking
        {
            for (size_t i = 0; i < nPeaks[n]; ++i)
            {
                // Find peak phase rotation
                size_t p = peak_loc[n][i];
                float h_phase_incr = princArg(phi[n][p] - phi[n - 1][p] - omega[p] * nHopA);
                inst_freq[n][p] = omega[p] + h_phase_incr / nHopA;
                // Find bins around peak
                if (nPeaks[n] == 1)
                {
                    bin_low[n][i] = 1;
                    bin_high[n][i] = nBins;
                }
                else if (i == 0)
                {
                    bin_low[n][i] = 1;
                    bin_high[n][i] = std::round((peak_loc[n][i + 1] + p) * 0.5);
                }
                else if (i == nPeaks[n] - 1)
                {
                    bin_low[n][i] = std::round((peak_loc[n][i - 1] + p) * 0.5);
                    bin_high[n][i] = nBins;
                }
                else
                {
                    bin_low[n][i] = std::round((peak_loc[n][i - 1] + p) * 0.5);
                    bin_high[n][i] = std::round((peak_loc[n][i + 1] + p) * 0.5);
                }
            }
        }
        else
        {
            Vec1D h_phase_incr(nBins, 0.0f);
            for (size_t p = 0; p < nBins; ++p)
            {
                h_phase_incr[p] = princArg(phi[n][p] - phi[n - 1][p] - nHopA * omega[p]);
                inst_freq[n][p] = omega[p] + h_phase_incr[p] / nHopA;
            }
        }
    }

    return std::make_tuple(inst_freq, bin_low, bin_high);
}



inline std::tuple<Vec2D, Vec2D, Vec1D> transientPreserve(Vec2D r_synth, Vec2D r, Vec2D Rt, Vec1D Rt_d, int max_decay_length, float ola_coef, float transient_threshold)
{
    // reset_frame = zeros(1, nFrames);
    // bins_to_reset = false(nBins, nFrames);
    
    size_t nFrames = r_synth.size();
    size_t nBins = r_synth[0].size();

    Vec2D bins_to_reset(nFrames, Vec1D(nBins, 0.0f));
    Vec1D reset_frame(nFrames, 0.0f);

    

    int attack_detected = 0;
    int decay_detected = 0;
    int decay_length = 0;

    std::vector<bool> decay_bins(nBins, false);
    std::vector<bool> decay_gains(nBins, true);
    std::vector<bool> attack_bins(nBins, false);
    std::vector<bool> attack_gains(nBins, true);



    for (size_t n = 0; n < nFrames; ++n) {
        DBG("nFrames: " << n);
        // Transient detection
        if (decay_detected)
        {
            // Remove bins from set transient bins during decay
            decay_length++;
            for (size_t i = 0; i < nBins; ++i)
            {
                if (decay_bins[i] && Rt[n][i] > 0.5)
                {
                    decay_bins[i] = true;
                    decay_gains[i] = 1.0f - Rt[n][i];
                }
                else
                {
                    decay_bins[i] = false;
                }
            }

            if (std::accumulate(decay_bins.begin(), decay_bins.end(), 0) == 0 || decay_length >= max_decay_length)
            {
                decay_detected = false;
                decay_length = 0;
            }
        }
        
        // Find local maxima from transientness
        if (n > 1) {
            if ((Rt_d[n] <= Rt_d[n - 1]) && (Rt_d[n - 1] > Rt_d[n - 2]) && (Rt_d[n - 1] > transient_threshold)) 
            {
                // Transient onset detected
                attack_detected = 1;
                for (size_t i = 0; i < nBins; ++i) 
                {
                    decay_bins[i] = 0;
                }
                decay_detected = 0;
            }
        }

        if (attack_detected ==1) 
        {
            // Check for transient center
            if ((Rt_d[n] <= 0) && (Rt_d[n - 1] > 0)) 
            {
                reset_frame[n] = n;
                for (size_t i = 0; i < nBins; ++i) 
                {
                    attack_bins[i] = 0;
                }
                attack_detected = 0;
            }

            // Add bins to set of transient bins during onset
            for (size_t i = 0; i < nBins; ++i) 
            {
                bins_to_reset[n][i] = attack_bins[i] || (Rt[n][i] > 0.5);
                //bins_to_reset[n][i] = bins_to_reset[n][i] || (Rt[n][i] > 0.5);
            }
            for (size_t i = 0; i < nBins; ++i) 
            {
                attack_gains[i] = 1 - Rt[n][i];
            }
        }
        // Synthesis magnitude spectrum           
        for (size_t i = 0; i < nBins; ++i)
        {
            r_synth[n][i] = r[n][i];
        }

        // Modify amplitude if transient onset/offset
        if (decay_detected)
        {
            for (size_t i = 0; i < nBins; ++i)
            {
                if (decay_bins[i])
                {
                    r_synth[n][i] = r_synth[n][i] * decay_gains[i];
                }
            }
        }

        
        // Phase reset
        if (reset_frame[n] == n) 
        {
            decay_detected = 1;

           /* Vec1D Rt_bin_to_reset(nBins, 0);
            for (size_t j = 0; j < nBins; ++j) {
                if (bins_to_reset[n][j]) {
                    Rt_bin_to_reset[j] = Rt[n][j];
                }
            }*/
            //r_synth[n][i] = r[n][i] * ola_coef * std::accumulate(Rt_bin_to_reset.begin(), Rt_bin_to_reset.end(), 0.0f) / Rt_bin_to_reset.size(); // mean(Rt_frame)
            for (size_t i = 0; i < nBins; ++i)
            {
                if (bins_to_reset[n][i])
                {
                    decay_bins[i] = true;
                    decay_gains[i] = 1.0f;
                }
                else
                {
                    decay_bins[i] = false;
                }
            }

            float mean_Rt = 0.0f;
            int count = 0;

            for (size_t i = 0; i < nBins; ++i)
            {
                if (bins_to_reset[n][i])
                {
                    mean_Rt += Rt[n][i];
                    count++;
                }
            }

            if (count > 0)
            {
                mean_Rt /= count;
            }

            for (size_t i = 0; i < nBins; ++i)
            {
                if (decay_bins[i])
                {
                    r_synth[n][i] = r[n][i] * ola_coef * mean_Rt;
                }
            }
        }
    }
    return std::make_tuple(r_synth, bins_to_reset, reset_frame);
}


inline std::pair<Vec2D, Vec1D> frame_transientness_PV(Vec2D T_matrix, float hopSizeAna)
{
    // the T_matrix comes in full size i.e. nFrames x NFFT
    // example
    // T_matrix.size(): 57 frames
    // T_matrix[0].size(): 8192 bins
    // half_T_matrix.size(): 57 frames
    // half_T_matrix[0].size(): 4097 bins

    size_t nFrames = T_matrix.size();
    size_t NFFT = T_matrix[0].size();
    size_t nBins = (NFFT / 2) + 1;

    Vec2D Rt(nFrames, std::vector<float>(nBins));

    for (size_t col = 0; col < nFrames; ++col)
    {
        for (size_t row = 0; row < nBins; ++row)
        {
            Rt[col][row] = T_matrix[col][row];
        }
    }


    // Frame transientness    

    Vec1D Rt_mean(nFrames);

    // Compute the mean along the first dimension (rows)
    for (int frame = 0; frame < nFrames; ++frame) {
        float sum = 0.0f;
        for (int bin = 0; bin < nBins; ++bin) {
            sum += Rt[frame][bin];
        }
        Rt_mean[frame] = sum / static_cast<float>(nBins);
    }

    // Time derivative of Rt
    Vec1D Rt_0(Rt_mean.size(), 0.0f);
    for (size_t i = 1; i < Rt_0.size(); ++i)
    {
        Rt_0[i] = Rt_mean[i - 1];
    }

    Vec1D Rt_d(Rt_mean.size(), 0.0f);
    for (int i = 0; i < Rt_mean.size(); ++i)
    {
        Rt_d[i] = (Rt_mean[i] - Rt_0[i]) / hopSizeAna;
    }


    return std::make_pair(Rt, Rt_d);

}


Vec2DComplex Fuzzy_PV_whole( Vec2DComplex& stftBuffer, int hopSizeAna, int hopSizeSyn, Vec2D& N_matrix1, Vec2D& r_synth, Vec2D& phi, Vec1D& psi, Vec1D& nPeaks, Vec2D& peak_loc,
    Vec2D& inst_freq, Vec2D& bin_low, Vec2D& bin_high, Vec2D& bins_to_reset, Vec1D& reset_frame)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-0.5, 0.5);

    size_t nFrames = stftBuffer.size();
    size_t NFFT = stftBuffer[0].size();
    size_t nBins = (NFFT / 2) + 1;
    Vec2DComplex Y(nFrames, std::vector<std::complex<float>>(NFFT));
    //Vec1D psi(nBins, 0.0f);
    
    float noise_coef = 1.0f / 4.0f * std::tanh(4.0f * (hopSizeSyn / hopSizeAna - 1.5)) + 1.0f;
    for (int n = 0; n < nFrames; ++n)
    {
        if (n == 0) // First frame, use analysis phase
        {
            for (size_t j = 0; j < nBins; ++j)
            {
                psi[j] = phi[n][j];
            }
        }
        else if (nPeaks[n] > 0) // Phase locking
        {
            for (size_t i = 0; i < nPeaks[n]; ++i)
            {
                size_t p = static_cast<size_t>(peak_loc[n][i]);
                float p_phase_rotation = princArg(psi[p] + hopSizeSyn * inst_freq[n][p] - phi[n][p]);

                for (size_t j = bin_low[n][i]; j < bin_high[n][i]; ++j)
                {
                    psi[j] = phi[n][j] + p_phase_rotation;

                    // Phase randomization
                    float An = (std::tanh(4.0f * (N_matrix1[n][j] - 1.0f)) + 1.0f) * noise_coef;
                    float phase_noise_value = j_Pi * An * 0.01;// dis(gen);
                    psi[j] = princArg(psi[j] + phase_noise_value);
                }
            }
        }
        else // No peaks found, standard PV processing
        {
            for (size_t j = 0; j < nBins; ++j)
            {
                psi[j] = princArg(psi[j] + hopSizeSyn * inst_freq[n][j]);
            }
        }

        // Phase reset
        if (reset_frame[n] == n)
        {
            for (size_t i = 0; i < nBins; ++i)
            {
                if (bins_to_reset[n][i])
                {
                    psi[i] = phi[n][i];
                }
            }
        }

        // Update
        for (size_t j = 0; j < nBins; ++j)
        {
            Y[n][j] = r_synth[n][j] * std::exp(std::complex<float>(0.0f, psi[j]));
        }

        // Apply complex conjugate and flipping
        for (size_t j = nBins + 1; j < NFFT; ++j)
        {
            Y[n][j] = std::conj(Y[n][NFFT - j]);
        }
    }
    return Y;
}



//================== DONT FORGET TO MAKE THE COMPLEX CONJUGATE for r_synth !!!!! ===============//


inline Vec2DComplex FuzzyPV(Vec2DComplex& X, int hopSizeAna, int hopSizeSyn, Vec2D T_matrix1, Vec2D N_matrix1, float transient_thresh, int maxDecayLen, float ola_coef)
{
    

    size_t NFFT = X[0].size();
    size_t nBins = (NFFT / 2) + 1;
    size_t nFrames = X.size();
    Vec2DComplex Y(nFrames, std::vector<std::complex<float>>(nBins));


    // Extracting out just first half (nBins)
    Vec2D half_T_matrix1(T_matrix1.size(), std::vector<float>(nBins));
    Vec2D half_N_matrix1(N_matrix1.size(), std::vector<float>(nBins));
    Vec2DComplex half_X(X.size(), std::vector<std::complex<float>>(nBins));



    // Extract the subset of Rt
    for (int col = 0; col < T_matrix1.size(); ++col) {
        for (int row = 0; row < nBins; ++row) {
            half_T_matrix1[col][row] = T_matrix1[col][row];
            half_N_matrix1[col][row] = N_matrix1[col][row];
            half_X[col][row] = X[col][row];
        }
    }


    // Frame transientness    
    auto Rt = half_T_matrix1;
    auto Rn = half_N_matrix1;
    std::vector<float> Rt_mean(nFrames);

    // Compute the mean along the first dimension (rows)
    for (int frame = 0; frame < nFrames; ++frame) {
        float sum = 0.0f;
        for (int bin = 0; bin < nBins; ++bin) {
            sum += Rt[frame][bin];
        }
        Rt_mean[frame] = sum / static_cast<float>(nBins);
    }

    // Time derivative of Rt
    std::vector<float> Rt_0(Rt_mean.size(), 0.0f);
    for (size_t i = 1; i < Rt_0.size(); ++i)
    {
        Rt_0[i] = Rt_mean[i - 1];
    }

    std::vector<float> Rt_d(Rt_mean.size(), 0.0f);
    for (int i = 0; i < Rt_mean.size(); ++i)
    {
        Rt_d[i] = (Rt_mean[i] - Rt_0[i]) / static_cast<float>(hopSizeAna);
    }

    // PV variables
    std::vector<float> omega(nBins);
    for (int k = 0; k < nBins; ++k)
    {
        omega[k] = 2 * j_Pi * k / NFFT;
    }

    //std::vector<float> omega(K, 0.0f);
    //for (int k = 0; k < (int)omega.size(); ++k)
    //    omega[k] = (float)k * fs / (float)N;

    // Transient detection / preservation variables
    int attack_detected = 0;
    int decay_detected = 0;
    std::vector<bool> decay_bins(nBins, false);
    std::vector<float> decay_gains(nBins, 1.0f);
    int decay_length = 0;
    std::vector<bool> attack_bins(nBins, false);
    std::vector<float> attack_gains(nBins, 1.0f);
    std::vector<bool> bins_to_reset(nBins, false);
    int reset_frame = 0;

    // Noise coef based on TSM factor
    float noise_coef = 1 / 4 * std::tanh(4 * (hopSizeSyn / hopSizeAna - 1.5)) + 1;


    // Main loop
    std::vector<float> r(nBins);
    std::vector<float> phi(nBins);
    std::vector<float> phi0(nBins, 0.0f);
    std::vector<float> psi(nBins);
    std::vector<float> Rt_frame(nBins);
    std::vector<int> peak_loc(nBins, 0);
    int nPeaks = 0;
    //DBG("\n");
    //DBG("========================Main Loop START========================");
    for (int n = 0; n < nFrames; ++n)
    {
        // Current frame spectrum
        std::vector<std::complex<float>> f = half_X[n];
        ////DBG("f.size(): " << f.size());
        for (int i = 0; i < nBins; ++i)
        {
            r[i] = std::abs(f[i]);
            phi[i] = std::arg(f[i]); // angle
        }

        // Frame transientness information
        Rt_frame = half_T_matrix1[n]; //     Rt(:, n);

        // Find spectral peaks
        nPeaks = 0;
        size_t i = 2;
        while (i < (size_t)nBins - 2)
        {
            if (r[i] > r[i - 1] && r[i] > r[i - 2] && r[i] > r[i + 1] && r[i] > r[i + 2])
            {
                nPeaks++;
                peak_loc[nPeaks] = i;
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
            for (int i = 1; i < nPeaks - 1; ++i)
            {

                // Find peak phase rotation
                int p = peak_loc[i];
                float h_phase_incr = princArg(phi[p] - phi0[p] - omega[p] * hopSizeAna);
                float inst_freq = omega[p] + h_phase_incr / hopSizeAna;
                float p_phase_rotation = princArg(psi[p] + hopSizeSyn * inst_freq - phi[p]);
                //DBG("Done Find peak phase rotation");

                // Find bins around peak
                //DBG("Start Find bins around peak");
                int bin_low, bin_high;
                if (nPeaks == 1)
                {
                    bin_low = 0;
                    bin_high = nBins - 1;
                }
                else if (i == 1)
                {
                    bin_low = 0;
                    bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
                }
                else if (i == nPeaks)
                {
                    bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                    bin_high = nBins - 1;
                }
                else
                {
                    bin_low = std::round((peak_loc[i - 1] + p) * 0.5);
                    bin_high = std::round((peak_loc[i + 1] + p) * 0.5);
                }

                // Rotate phases according to peak rotation
                for (int j = bin_low; j <= bin_high; ++j)
                {
                    psi[j] = phi[j] + p_phase_rotation;

                    // Phase randomization
                    float An = (std::tanh(4 * (Rn[n][j] - 1)) + 1) * noise_coef;

                    //std::srand(static_cast<unsigned int>(std::time(nullptr))); // Set seed using current time
                    //DBG("Check random: " << (std::rand() / static_cast<float>(RAND_MAX) - 0.5f));
                    float phase_noise_value = j_Pi * An * (std::rand() / static_cast<float>(RAND_MAX) - 0.5f);
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

        // Transient detection
        if (decay_detected) // i.e. decay_detected != 0
        {
            // Remove bins from set transient bins during decay
            decay_length++;
            for (int i = 0; i < nBins; ++i)
            {
                decay_bins[i] = decay_bins[i] && (Rt_frame[i] > 0.5);
            }

            if (std::accumulate(decay_bins.begin(), decay_bins.end(), 0) == 0 || decay_length >= maxDecayLen)
            {
                decay_detected = 0;
                decay_length = 0;
            }
            else
            {
                for (int i = 0; i < nBins; ++i)
                {
                    decay_gains[i] = 1 - Rt_frame[i];
                }
            }
        }

        // Find local maxima from transientness
        if (n > 1)
        {
            if ((Rt_d[n] <= Rt_d[n - 1] && Rt_d[n - 1] > Rt_d[n - 2]) && Rt_d[n - 1] > transient_thresh)
            {
                // Transient onset detected
                attack_detected = 1;
                std::fill(attack_bins.begin(), attack_bins.end(), 0);
                decay_detected = 0;
            }
        }

        if (attack_detected)
        {
            // Check for transient center
            if (Rt_d[n] <= 0 && Rt_d[n - 1] > 0)
            {
                reset_frame = n;
                std::fill(attack_bins.begin(), attack_bins.end(), 0);
                attack_detected = 0;
            }

            // Add bins to set of transient bins during onset
            for (int i = 0; i < nBins; ++i)
            {
                bins_to_reset[i] = attack_bins[i] || (Rt_frame[i] > 0.5);
            }

            for (int i = 0; i < nBins; ++i)
            {
                attack_gains[i] = 1 - Rt_frame[i];
            }
        }


        // Synthesis magnitude spectrum
        // 
        std::vector<float> r_synth = r;
        //std::vector<float> r_synth(nBins);
        // Modify amplitude if transient onset/offset
        for (int i = 0; i < nBins; ++i)
        {
            r_synth[i] = r[i];
            if (decay_detected && decay_bins[i])
            {
                r_synth[i] *= decay_gains[i];
            }
            else if (attack_detected && attack_bins[i])
            {
                r_synth[i] *= attack_gains[i];
            }
        }

        // Phase reset
        if (reset_frame == n)
        {
            decay_detected = 1;
            for (int i = 0; i < nBins; ++i)
            {
                decay_bins[i] = bins_to_reset[i];
                decay_gains[i] = 1.0f;
                r_synth[i] = r[i] * ola_coef * std::accumulate(Rt_frame.begin(), Rt_frame.end(), 0.0f) / Rt_frame.size(); // mean(Rt_frame)
                psi[i] = phi[i];
            }
            reset_frame = 0;
            std::fill(bins_to_reset.begin(), bins_to_reset.end(), 0);
        }

        // Update
        for (int i = 0; i < nBins; ++i)
        {
            Y[n][i] = r_synth[i] * std::exp(std::complex<float>(0, psi[i]));
            phi0[i] = phi[i];
        }

    }


    return Y;
}





/** Compares greater/less than with absolute value */
template<typename T>
static bool abs_compare(T a, T b)
{
    return (std::abs(a) < std::abs(b));
}
/** Returns the maximum absolute value in a vector */
static float max_abs(const std::vector<float>& vec)
{
    return std::abs(*std::max_element(vec.begin(), vec.end(), abs_compare<float>));
}

std::vector<size_t> findPeaks(const Vec1D& src, int distance, float minHeight)
{
    size_t length = src.size();
    if (length <= 1)
        return std::vector<size_t>();

    std::vector<int> sign(length, -1);
    std::vector<float> difference(length, 0.f);
    std::vector<size_t> temp_out;

    adjacent_difference(src.begin(), src.end(), difference.begin());
    difference.erase(difference.begin());
    difference.pop_back();

    for (int i = 0; i < difference.size(); ++i)
    {
        if (difference[i] >= 0.f)
            sign[i] = 1;
    }

    for (int j = 1; j < length - 1; ++j)
    {
        int diff = sign[j] - sign[j - 1];
        if (diff < 0)
        {
            temp_out.push_back(j);
        }
    }

    if (temp_out.size() == 0 || distance == 0)
        return temp_out;

    sort(temp_out.begin(), temp_out.end(), [&src](size_t a, size_t b) {
        return (std::abs(src[a]) > std::abs(src[b]));
        });

    std::vector<size_t> ans;
    std::unordered_map<size_t, int> except;

    for (auto it : temp_out)
    {
        //DBG("Checking element " << it << ", value: " << src[it]);
        if (!except.count(it) && src[it] >= minHeight)
        {
            ans.push_back(it);
            size_t left = it - distance > 0 ? it - distance : 0;
            size_t right = it + distance > length - 1 ? length - 1 : it + distance;
            for (size_t i = left; i <= right; ++i)
                ++except[i];
        }
    }

    sort(ans.begin(), ans.end());

    

    DBG("length: " << length);
    DBG("temp_out size: " << temp_out.size());
    DBG("ans size: " << ans.size());



    return ans;
}

