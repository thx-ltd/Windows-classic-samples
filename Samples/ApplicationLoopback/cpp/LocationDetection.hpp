#pragma once

// Include complex for std::complex definition, which is used by fftw3.h.
// std::complex is used for types but this is binary compatible with both
// complex.h's double[2] and fftw_complex.
#include <complex>

// Include complex.h for fftw_complex definition, which is used by fftw3.h.
// complex.h must be included before fftw3.h to avoid compilation errors.
#include <complex.h>
#include <fftw3.h>

#include <memory>
#include <vector>
#include <cstdint>

struct LocationDetection
{
    LocationDetection() = delete;

    LocationDetection(int nSamplesSignal, int nChannelsSignal);

    LocationDetection(const LocationDetection &) = delete;

    LocationDetection &operator=(const LocationDetection &) = delete;

    ~LocationDetection() = default;

    void ProcessSignal(const float *signal, uint32_t FramesAvailable);

  private:
    /**
     * Number of channels in the signal.
     */
    uint32_t m_nChannelsSignal;

    /**
     * Number of samples in the kernel.
     *
     * int because fftw functions take int.
     */
    int m_nSamplesKernel;

    /**
     * m + n - 1 rounded up to the next power of 2, where m is the number of
     * samples in the kernel, n is the number of samples in signal.
     *
     *  int because fftw functions take int.
     */
    int m_nSamplesFFT;

    fftw_plan m_SignalFFTPlanForward;
    fftw_plan m_SignalFFTPlanBackward;

    using real_type    = double;
    using complex_type = std::complex<real_type>;

    /**
     * Deleter for memory allocated with fftw_alloc*
     */
    struct FFTWComplexDeleter
    {
        void operator()(complex_type *ptr) const { fftw_free(ptr); }
    };

    /**
     * Deleter for memory allocated with fftw_alloc*
     */
    struct FFTWRealDeleter
    {
        void operator()(real_type *ptr) const { fftw_free(ptr); }
    };

    using TimeDomainArray = std::unique_ptr<real_type, FFTWRealDeleter>;

    using FrequencyDomainArray =
        std::unique_ptr<complex_type, FFTWComplexDeleter>;

    /**
     * Kernel array in frequency domain sized for FFT, i.e., m + n - 1
     * rounded up to the next power of 2, where m is the size of the kernel
     * and n is the size of the signal.
     */
    FrequencyDomainArray m_KernelFFT;

    /**
     * 1D Signal array in time domain sized for FFT, i.e., m + n - 1 rounded
     * up to the next power of 2.
     */
    TimeDomainArray m_SignalTimeDomain;

    /**
     * Signal array in frequency domain sized for FFT, i.e., m + n - 1
     * rounded up to the next power of 2, where m is the size of the kernel
     * and n is the size of the signal.
     */
    FrequencyDomainArray m_SignalFrequencyDomain;

    const int    m_nPeaksToFind                   = 10;
    const double m_PeakThreshold                  = 0.5;
    const int    m_nMinSampleDistanceBetweenPeaks = 100;

    /**
     * Peak indices by channel.
     */
    std::vector<std::vector<int>> m_PeakIndices;

    /**
     * Peak values by channel.
     */
    std::vector<std::vector<double>> m_PeakValues;
};
