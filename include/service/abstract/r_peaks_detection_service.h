#ifndef EKG_R_PEAKS_SERVICE_H
#define EKG_R_PEAKS_SERVICE_H
#include <vector>

#include "../../model/r_peaks_annotated_signal_datapoint.h"
#include "../../model/signal_datapoint.h"

class IRPeaksDetectionService {
public:
    virtual ~IRPeaksDetectionService() = default;

    virtual std::vector<RPeaksAnnotatedSignalDatapoint> Detect(const std::vector<SignalDatapoint>& datapoints, int frequency) =
    0;
};

#endif //EKG_R_PEAKS_SERVICE_H

