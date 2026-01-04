#include "../../include/service/waves_detection_service.h"
#include "../../include/service/butterworth_filter_service.h"
#include "../../include/service/moving_average_filter_service.h"

#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <iostream>

namespace {
    struct BeatIndices {
        int r = -1;
        int p_on = -1;
        int p_end = -1;
        int qrs_on = -1;
        int qrs_end = -1;
        int t_end = -1;
    };

    double GetMean(const std::vector<double> &data, int start, int end) {
        if (start >= end) return 0.0;
        double sum = 0.0;
        int count = 0;
        int safe_end = std::min((int) data.size(), end);
        int safe_start = std::max(0, start);
        for (int i = safe_start; i < safe_end; ++i) {
            sum += data[i];
            count++;
        }
        return (count > 0) ? (sum / count) : 0.0;
    }

    double GetSlope(const std::vector<double> &data, int idx) {
        if (idx <= 0 || idx >= (int) data.size() - 1) return 0.0;
        return (data[idx + 1] - data[idx - 1]) * 0.5;
    }

    int FindMinIndex(const std::vector<double> &data, int start, int end) {
        int safe_start = std::max(0, start);
        int safe_end = std::min((int) data.size(), end);
        if (safe_start >= safe_end) return safe_start;

        double min_val = 1e9;
        int min_idx = safe_start;
        for (int i = safe_start; i < safe_end; ++i) {
            if (data[i] < min_val) {
                min_val = data[i];
                min_idx = i;
            }
        }
        return min_idx;
    }

    int GetTangentPoint(const std::vector<double> &signal, int anchor_idx, int win_len, double baseline,
                        int direction) {
        int start, end;
        int n = (int) signal.size();

        if (direction == -1) {
            start = std::max(0, anchor_idx - win_len);
            end = anchor_idx;
        } else {
            start = anchor_idx;
            end = std::min(n, anchor_idx + win_len);
        }

        double max_slope = 0.0;
        int steepest_idx = anchor_idx;

        for (int i = start; i < end; ++i) {
            double slope = std::abs(GetSlope(signal, i));
            if (slope > max_slope) {
                max_slope = slope;
                steepest_idx = i;
            }
        }

        if (max_slope < 1e-5) return anchor_idx;

        double y_steep = signal[steepest_idx];
        double m_steep = GetSlope(signal, steepest_idx);

        if (std::abs(m_steep) < 1e-9) return anchor_idx;

        double intersect_x = steepest_idx + (baseline - y_steep) / m_steep;
        return static_cast<int>(std::round(intersect_x));
    }

    void AnalyzePWave(const std::vector<double> &smooth_sig, int search_start, int search_end, double baseline,
                      int &p_on, int &p_end) {
        if (search_start >= search_end) {
            p_on = p_end = search_start;
            return;
        }

        int p_peak = search_start;
        double max_dist = -1.0;
        for (int i = search_start; i < search_end; ++i) {
            double dist = std::abs(smooth_sig[i] - baseline);
            if (dist > max_dist) {
                max_dist = dist;
                p_peak = i;
            }
        }

        if (max_dist < 0.01) {
            p_on = p_end = search_start;
            return;
        }

        double max_slope_left = 0.0;
        int flank_left = p_peak;
        for (int i = p_peak; i > search_start; --i) {
            double s = std::abs(GetSlope(smooth_sig, i));
            if (s > max_slope_left) {
                max_slope_left = s;
                flank_left = i;
            }
        }
        double thr_left = max_slope_left * 0.15;
        p_on = flank_left;
        for (int i = flank_left; i > search_start; --i) {
            if (std::abs(GetSlope(smooth_sig, i)) < thr_left) {
                p_on = i;
                break;
            }
        }

        double max_slope_right = 0.0;
        int flank_right = p_peak;
        for (int i = p_peak; i < search_end; ++i) {
            double s = std::abs(GetSlope(smooth_sig, i));
            if (s > max_slope_right) {
                max_slope_right = s;
                flank_right = i;
            }
        }
        double thr_right = max_slope_right * 0.15;
        p_end = flank_right;
        for (int i = flank_right; i < search_end; ++i) {
            if (std::abs(GetSlope(smooth_sig, i)) < thr_right) {
                p_end = i;
                break;
            }
        }
    }

    int CalculateTEndIntersection(const std::vector<double> &signal, const std::vector<double> &smooth_sig,
                                  int s_point, int limit_point, double baseline, int fs) {
        int search_start = s_point + static_cast<int>(0.04 * fs);
        int search_end = std::min(limit_point, (int) signal.size());

        if (search_start >= search_end) return limit_point;

        int t_peak_idx = search_start;
        double max_amp = -1.0;
        for (int i = search_start; i < search_end; ++i) {
            double amp = std::abs(smooth_sig[i] - baseline);
            if (amp > max_amp) {
                max_amp = amp;
                t_peak_idx = i;
            }
        }

        if (max_amp < 0.02) return std::min(s_point + (int) (0.2 * fs), limit_point);

        int slope_search_end = std::min(limit_point, t_peak_idx + static_cast<int>(0.25 * fs));
        double max_downslope = 0.0;
        int steepest_idx = t_peak_idx;

        for (int i = t_peak_idx; i < slope_search_end; ++i) {
            double s = std::abs(GetSlope(smooth_sig, i));
            if (s > max_downslope) {
                max_downslope = s;
                steepest_idx = i;
            }
        }

        if (max_downslope < 1e-5) return std::min(t_peak_idx + (int) (0.1 * fs), limit_point);

        double y0 = smooth_sig[steepest_idx];
        double m = GetSlope(smooth_sig, steepest_idx);
        if (std::abs(m) < 1e-9) return limit_point;

        double intersect_x = steepest_idx + (baseline - y0) / m;
        int t_end_candidate = static_cast<int>(std::round(intersect_x));

        if (t_end_candidate <= t_peak_idx) t_end_candidate = t_peak_idx + 10;
        if (t_end_candidate > limit_point) t_end_candidate = limit_point;

        return t_end_candidate;
    }
}


std::vector<WaveAnnotatedSignalDatapoint> WavesDetectionService::Detect(const std::vector<SignalDatapoint> &datapoints,
                                                                        int frequency) {
    if (datapoints.empty()) {
        return {};
    }

    ButterworthFilterService butterworthFilter;
    MovingAverageFilterService movingAvgFilter;

    auto filteredPoints = butterworthFilter.Filter(datapoints);
    auto smoothedPoints = movingAvgFilter.Filter(filteredPoints);

    std::vector<double> signal;
    std::vector<double> sig_smooth;
    signal.reserve(filteredPoints.size());
    sig_smooth.reserve(smoothedPoints.size());

    size_t channelIdx = 0;
    if (!filteredPoints.empty() && filteredPoints[0].channelValues.size() > 1) channelIdx = 1;

    for (const auto &p: filteredPoints) {
        if (p.channelValues.size() > channelIdx) signal.push_back(p.channelValues[channelIdx]);
        else signal.push_back(0.0);
    }
    for (const auto &p: smoothedPoints) {
        if (p.channelValues.size() > channelIdx) sig_smooth.push_back(p.channelValues[channelIdx]);
        else sig_smooth.push_back(0.0);
    }

    std::vector<int> r_peaks;
    {
        double max_val = -1e9;
        for (double v: signal) if (v > max_val) max_val = v;
        double threshold = max_val * 0.5;
        int min_dist = (int) (0.6 * frequency);
        int last_r = -min_dist;

        for (size_t i = 1; i < signal.size() - 1; ++i) {
            if (signal[i] > threshold && signal[i] > signal[i - 1] && signal[i] > signal[i + 1]) {
                if ((int) i - last_r > min_dist) {
                    int win = std::max(1, frequency / 50);
                    int start = std::max(0, (int) i - win);
                    int end = std::min((int) signal.size(), (int) i + win);
                    int real_r = i;
                    double local_max = -1e9;
                    for (int k = start; k < end; ++k) {
                        if (signal[k] > local_max) {
                            local_max = signal[k];
                            real_r = k;
                        }
                    }
                    r_peaks.push_back(real_r);
                    last_r = real_r;
                }
            }
        }
    }

    std::vector<BeatIndices> beats(r_peaks.size());
    std::vector<int> p_onsets(r_peaks.size(), 0);

    int win_qs = static_cast<int>(0.06 * frequency);
    int win_tan = static_cast<int>(0.03 * frequency);
    int win_p_look = static_cast<int>(0.24 * frequency);

    for (size_t i = 0; i < r_peaks.size(); ++i) {
        int r = r_peaks[i];
        beats[i].r = r;

        int bl_start = std::max(0, r - static_cast<int>(0.10 * frequency));
        int bl_end = std::max(0, r - static_cast<int>(0.04 * frequency));
        double baseline = GetMean(signal, bl_start, bl_end);

        int q_min = FindMinIndex(signal, std::max(0, r - win_qs), r);
        beats[i].qrs_on = GetTangentPoint(signal, q_min, win_tan, baseline, -1);
        if (beats[i].qrs_on > q_min) beats[i].qrs_on = q_min - 2;

        int s_min = FindMinIndex(signal, r, std::min((int) signal.size(), r + win_qs));
        beats[i].qrs_end = GetTangentPoint(signal, s_min, win_tan, baseline, 1);
        if (beats[i].qrs_end < s_min) beats[i].qrs_end = s_min + 2;

        int p_search_end = beats[i].qrs_on - static_cast<int>(0.02 * frequency);
        int p_search_start = std::max(0, beats[i].qrs_on - win_p_look);
        AnalyzePWave(sig_smooth, p_search_start, p_search_end, baseline, beats[i].p_on, beats[i].p_end);

        p_onsets[i] = beats[i].p_on;
    }

    for (size_t i = 0; i < r_peaks.size(); ++i) {
        int s_point = beats[i].qrs_end;
        int limit;
        if (i < r_peaks.size() - 1) {
            limit = std::max(s_point + 10, p_onsets[i + 1] - static_cast<int>(0.02 * frequency));
        } else {
            limit = std::min((int) signal.size() - 1, s_point + static_cast<int>(0.5 * frequency));
        }

        int bl_start = std::max(0, beats[i].r - static_cast<int>(0.10 * frequency));
        int bl_end = std::max(0, beats[i].r - static_cast<int>(0.04 * frequency));
        double baseline = GetMean(signal, bl_start, bl_end);

        beats[i].t_end = CalculateTEndIntersection(signal, sig_smooth, s_point, limit, baseline, frequency);
    }

    std::vector<WaveAnnotatedSignalDatapoint> result;
    result.reserve(datapoints.size());

    std::vector<int> p_on_map(signal.size(), 0);
    std::vector<int> p_end_map(signal.size(), 0);
    std::vector<int> qrs_on_map(signal.size(), 0);
    std::vector<int> qrs_end_map(signal.size(), 0);
    std::vector<int> t_end_map(signal.size(), 0);

    for (const auto &b: beats) {
        if (b.p_on >= 0) p_on_map[b.p_on] = 1;
        if (b.p_end >= 0) p_end_map[b.p_end] = 1;
        if (b.qrs_on >= 0) qrs_on_map[b.qrs_on] = 1;
        if (b.qrs_end >= 0) qrs_end_map[b.qrs_end] = 1;
        if (b.t_end >= 0) t_end_map[b.t_end] = 1;
    }

    for (size_t i = 0; i < datapoints.size(); ++i) {
        WaveAnnotatedSignalDatapoint wp;
        wp.channelValues = datapoints[i].channelValues;

        if (p_on_map[i]) wp.p_wave_onset = true;
        if (p_end_map[i]) wp.p_wave_end = true;
        if (qrs_on_map[i]) wp.qrs_onset = true;
        if (qrs_end_map[i]) wp.qrs_end = true;
        if (t_end_map[i]) wp.t_end = true;

        result.push_back(wp);
    }

    return result;
}
