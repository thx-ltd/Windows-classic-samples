#pragma once

#include <AudioClient.h>
#include <guiddef.h>
#include <initguid.h>
#include <mfapi.h>
#include <mmdeviceapi.h>

#include <wil\com.h>
#include <wil\result.h>
#include <wrl\implements.h>

#include "Common.h"

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

using namespace Microsoft::WRL;

class CLoopbackCapture
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, FtmBase,
                          IActivateAudioInterfaceCompletionHandler>
{
  public:
    CLoopbackCapture() = default;
    ~CLoopbackCapture();

    HRESULT StartCaptureAsync(DWORD processId, bool includeProcessTree);
    HRESULT StopCaptureAsync();

    METHODASYNCCALLBACK(CLoopbackCapture, StartCapture, OnStartCapture);
    METHODASYNCCALLBACK(CLoopbackCapture, StopCapture, OnStopCapture);
    METHODASYNCCALLBACK(CLoopbackCapture, SampleReady, OnSampleReady);
    METHODASYNCCALLBACK(CLoopbackCapture, FinishCapture, OnFinishCapture);

    // IActivateAudioInterfaceCompletionHandler
    STDMETHOD(ActivateCompleted)(
        IActivateAudioInterfaceAsyncOperation *operation);

  private:
    // NB: All states >= Initialized will allow some methods
    // to be called successfully on the Audio Client
    enum class DeviceState
    {
        Uninitialized,
        Error,
        Initialized,
        Starting,
        Capturing,
        Stopping,
        Stopped,
    };

    HRESULT OnStartCapture(IMFAsyncResult *pResult);
    HRESULT OnStopCapture(IMFAsyncResult *pResult);
    HRESULT OnFinishCapture(IMFAsyncResult *pResult);
    HRESULT OnSampleReady(IMFAsyncResult *pResult);

    HRESULT InitializeLoopbackCapture();
    HRESULT OnAudioSampleRequested();

    HRESULT ActivateAudioInterface(DWORD processId, bool includeProcessTree);
    HRESULT FinishCaptureAsync();

    HRESULT SetDeviceStateErrorIfFailed(HRESULT hr);

    wil::com_ptr_nothrow<IAudioClient> m_AudioClient;
    WAVEFORMATEXTENSIBLE               m_CaptureFormat{};

    /**
     * Number of samples per channel in the signal.
     */
    UINT32 m_BufferFrames = 0;

    wil::com_ptr_nothrow<IAudioCaptureClient> m_AudioCaptureClient;
    wil::com_ptr_nothrow<IMFAsyncResult>      m_SampleReadyAsyncResult;

    wil::unique_event_nothrow m_SampleReadyEvent;
    MFWORKITEM_KEY            m_SampleReadyKey = 0;
    wil::critical_section     m_CritSec;
    DWORD                     m_dwQueueID  = 0;
    DWORD                     m_cbDataSize = 0;

    // These two members are used to communicate between the main thread
    // and the ActivateCompleted callback.
    HRESULT m_activateResult = E_UNEXPECTED;

    DeviceState               m_DeviceState{DeviceState::Uninitialized};
    wil::unique_event_nothrow m_hActivateCompleted;
    wil::unique_event_nothrow m_hCaptureStopped;

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

    using real_type = double;
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

    const int m_nPeaksToFind = 10;
    const double m_PeakThreshold = 0.5;
    const int    m_nMinSampleDistanceBetweenPeaks = 100;

    std::vector<int> m_PeakIndices;

    std::vector<double> m_PeakValues;
};
