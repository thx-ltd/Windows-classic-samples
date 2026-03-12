#include "thx_dsp.h"

void thx_find_peaks(const double *signal, int nSamples, double threshold,
                    int minDistance, int maxPeaks, int *peakIndices,
                    double *peakValues, int *nPeaksFound)
{
    int count = 0;
    for (int i = 1; i < nSamples - 1 && count < maxPeaks; ++i)
    {
        if (signal[i] > threshold && signal[i] > signal[i - 1] &&
            signal[i] > signal[i + 1])
        {
            // Check if this peak is sufficiently far from the last detected
            // peak
            if (count == 0 || i - peakIndices[count - 1] >= minDistance)
            {
                peakIndices[count] = i;
                peakValues[count]  = signal[i];
                count++;
            }
        }
    }
    *nPeaksFound = count;
}
