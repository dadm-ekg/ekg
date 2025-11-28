#ifndef EKG_R_PEAKS_SERVICE_H
#define EKG_R_PEAKS_SERVICE_H
#include <vector>

#include "../../model/r_peaks_annotated_signal_datapoint.h"
#include "../../model/signal_datapoint.h"

enum class RPeaksDetectionMethod {
    PanTompkins,
    Hilbert,
    Wavelet
};

class IRPeaksDetectionService {
public:
    virtual ~IRPeaksDetectionService() = default;

    virtual std::vector<RPeaksAnnotatedSignalDatapoint> Detect(const std::vector<SignalDatapoint> &datapoints,
                                                               int frequency,
                                                               RPeaksDetectionMethod method = RPeaksDetectionMethod::PanTompkins) =
    0;
};

#endif //EKG_R_PEAKS_SERVICE_H
