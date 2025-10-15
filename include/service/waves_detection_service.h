#ifndef EKG_WAVES_DETECTION_SERVICE_IMPL_H
#define EKG_WAVES_DETECTION_SERVICE_IMPL_H

#include "abstract/waves_detection_service.h"

class WavesDetectionService : public IWavesDetectionService {
public:
    std::vector<WaveAnnotatedSignalDatapoint> Detect(const std::vector<SignalDatapoint>& datapoints, int frequency) override;
};

#endif //EKG_WAVES_DETECTION_SERVICE_IMPL_H
