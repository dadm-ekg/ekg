#ifndef EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
#define EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
#include "signal_datapoint.h"
#include <vector>

class RPeaksAnnotatedSignalDatapoint : public SignalDatapoint {
public:
    std::vector<bool> peaks;
    
    bool HasAnyPeak() const {
        for (bool p : peaks) {
            if (p) return true;
        }
        return false;
    }
    
    bool HasPeak(int channel) const {
        if (channel < 0 || static_cast<size_t>(channel) >= peaks.size()) {
            return false;
        }
        return peaks[static_cast<size_t>(channel)];
    }
};

#endif //EKG_R_PEAKS_ANNOTATED_SIGNAL_DATAPOINT_H
