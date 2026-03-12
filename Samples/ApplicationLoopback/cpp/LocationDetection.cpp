#include "LocationDetection.hpp"

#include "thx_dsp.h"

#include <cstdint>
#include <cmath>
#include <complex.h>
#include <complex>
#include <fftw3.h>

LocationDetection::LocationDetection(int nSamplesSignal, int nChannelsSignal)
    : m_nChannelsSignal(nChannelsSignal)
{
    /// TODO Load kernel - m
    m_nSamplesKernel = 1024;

    m_nSamplesFFT = nSamplesSignal + m_nSamplesKernel - 1;
    // Round up to the next power of 2
    m_nSamplesFFT =
        1ULL << static_cast<int>(std::ceil(std::log2(m_nSamplesFFT)));

    std::unique_ptr<double> kernel(fftw_alloc_real(m_nSamplesFFT));

    // Allocate the kernel and kernel FFT arrays and FFT the kernel
    // No need to plan because this is only done once.
    m_KernelFFT.reset(
        reinterpret_cast<complex_type *>(fftw_alloc_complex(m_nSamplesFFT)));

    fftw_plan kernelPlan = fftw_plan_dft_r2c_1d(
        m_nSamplesFFT, kernel.get(),
        reinterpret_cast<fftw_complex *>(m_KernelFFT.get()), FFTW_ESTIMATE);

    // FFT the kernel
    fftw_execute_dft_r2c(kernelPlan, kernel.get(),
                         reinterpret_cast<fftw_complex *>(m_KernelFFT.get()));

    // Allocate the signal time domain array
    m_SignalTimeDomain.reset(fftw_alloc_real(m_nSamplesFFT));

    // Allocate the signal FFT array
    m_SignalFrequencyDomain.reset(
        reinterpret_cast<complex_type *>(fftw_alloc_complex(m_nSamplesFFT)));

    m_SignalFFTPlanForward = fftw_plan_dft_r2c_1d(
        m_nSamplesFFT, m_SignalTimeDomain.get(),
        reinterpret_cast<fftw_complex *>(m_SignalFrequencyDomain.get()),
        FFTW_ESTIMATE | FFTW_FORWARD);

    m_SignalFFTPlanBackward = fftw_plan_dft_c2r_1d(
        m_nSamplesFFT,
        reinterpret_cast<fftw_complex *>(m_SignalFrequencyDomain.get()),
        m_SignalTimeDomain.get(), FFTW_ESTIMATE | FFTW_BACKWARD);

    m_PeakIndices.resize(m_nChannelsSignal);
    m_PeakValues.resize(m_nChannelsSignal);
}

void LocationDetection::ProcessSignal(const float *signal, uint32_t FramesAvailable)
{
    /// Iterate through channels
    for (uint32_t channel = 0; channel < m_nChannelsSignal; ++channel)
    {
        m_PeakIndices[channel].clear();
        m_PeakIndices[channel].resize(m_nPeaksToFind);
        m_PeakValues[channel].clear();
        m_PeakValues[channel].resize(m_nPeaksToFind);

        // Copy from signal to m_SignalTimeDomain, only the samples for
        // this channel
        for (uint32_t frame = 0; frame < FramesAvailable; ++frame)
        {
            *(m_SignalTimeDomain.get() + frame) =
                signal[frame * m_nChannelsSignal + channel];
        }

        /// FFT signal
        fftw_execute(m_SignalFFTPlanForward);

        /// Frequency domain convolution of signal with kernel
        for (int i = 0; i < m_nSamplesFFT / 2 + 1; ++i)
        {
            std::complex<double> &signalValue(
                *(m_SignalFrequencyDomain.get() + i));
            std::complex<double> &kernelValue(*(m_KernelFFT.get() + i));
            std::complex<double>  convolvedValue = signalValue * kernelValue;
            *(m_SignalFrequencyDomain.get() + i) = convolvedValue;
        }

        /// IFFT convolved signal back to time domain
        fftw_execute(m_SignalFFTPlanBackward);

        int peaksFound = 0;

        /// Find peaks in convolution
        thx_find_peaks(m_SignalTimeDomain.get(), FramesAvailable,
                       /*threshold=*/m_PeakThreshold,
                       /*minDistance=*/m_nMinSampleDistanceBetweenPeaks,
                       /*maxPeaks=*/m_nPeaksToFind,
                       m_PeakIndices[channel].data(),
                       m_PeakValues[channel].data(), &peaksFound);

        m_PeakIndices[channel].resize(peaksFound);
        m_PeakValues[channel].resize(peaksFound);
    }

    /// TODO Find time aligned peaks in different channels to determine
    /// direction of sound

    /// TODO Ratio of peak heights in different channels gives the
    /// proportional distance from the channel locations.
}
