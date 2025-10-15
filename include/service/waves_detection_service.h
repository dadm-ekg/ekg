#ifndef EKG_WAVES_DETECTION_SERVICE_H
#define EKG_WAVES_DETECTION_SERVICE_H
#include <vector>

#include "../model/wave_annotated_signal_datapoint.h"

class IWavesDetectionService {
public:
    virtual ~IWavesDetectionService() = default;

    virtual std::vector<WaveAnnotatedSignalDatapoint> Detect(std::vector<SignalDatapoint> datapoints, int frequency) =
    0;
};

#endif //EKG_WAVES_DETECTION_SERVICE_H
