#ifndef EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#define EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
#include "signal_datapoint.h"
#include <vector>

class WaveAnnotatedSignalDatapoint : public SignalDatapoint {
public:
    std::vector<bool> p_wave_onset;
    std::vector<bool> p_wave_end;
    std::vector<bool> qrs_onset;
    std::vector<bool> qrs_end;
    std::vector<bool> t_end;
    
    bool HasAnyMarker() const {
        for (size_t ch = 0; ch < p_wave_onset.size(); ++ch) {
            if (ch < p_wave_onset.size() && p_wave_onset[ch]) return true;
            if (ch < p_wave_end.size() && p_wave_end[ch]) return true;
            if (ch < qrs_onset.size() && qrs_onset[ch]) return true;
            if (ch < qrs_end.size() && qrs_end[ch]) return true;
            if (ch < t_end.size() && t_end[ch]) return true;
        }
        return false;
    }
    
    bool HasMarker(int channel) const {
        if (channel < 0) return false;
        size_t ch = static_cast<size_t>(channel);
        return (ch < p_wave_onset.size() && p_wave_onset[ch]) ||
               (ch < p_wave_end.size() && p_wave_end[ch]) ||
               (ch < qrs_onset.size() && qrs_onset[ch]) ||
               (ch < qrs_end.size() && qrs_end[ch]) ||
               (ch < t_end.size() && t_end[ch]);
    }
};

#endif //EKG_WAVE_ANNOTATED_SIGNAL_DATAPOINT_H
