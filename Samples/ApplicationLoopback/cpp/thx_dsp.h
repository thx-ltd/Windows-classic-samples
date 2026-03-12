#pragma once

/**
 * @brief Finds peaks in a signal above a threshold.
 * @param signal Input signal samples.
 * @param nSamples Number of samples in the input signal.
 * @param threshold Minimum value for a peak to be considered.
 * @param minDistance Minimum distance between peaks (in samples).
 * @param maxPeaks Maximum number of peaks to return.
 * @param peakIndices Output array of peak indices (size at least maxPeaks).
 * @param peakValues Output array of peak values (size at least maxPeaks).
 * @param nPeaksFound Output number of peaks found.
 */
void thx_find_peaks(const double *signal, int nSamples, double threshold,
                    int minDistance, int maxPeaks, int *peakIndices,
                    double *peakValues, int *nPeaksFound);
