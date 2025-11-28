#ifndef EKG_R_PEAKS_DETECTION_SERVICE_H
#define EKG_R_PEAKS_DETECTION_SERVICE_H

#include "abstract/r_peaks_detection_service.h"

class RPeaksDetectionService : public IRPeaksDetectionService {
public:
    std::vector<RPeaksAnnotatedSignalDatapoint>
    Detect(const std::vector<SignalDatapoint> &datapoints, int frequency,
           RPeaksDetectionMethod method = RPeaksDetectionMethod::PanTompkins) override;
};

#endif //EKG_R_PEAKS_DETECTION_SERVICE_H
