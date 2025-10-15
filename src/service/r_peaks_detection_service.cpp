#include "../../include/service/r_peaks_detection_service.h"

class RPeaksDetectionService : public IRPeaksDetectionService {
public:
    std::vector<RPeaksAnnotatedSignalDatapoint>
    Detect(std::vector<SignalDatapoint> datapoints, int frequency) override {
        // TODO(Wiktor): trzeba uzupełnić. Ta metoda musi zwrócić wektor RPeaksAnnotatedSignalDatapoint z uzupełnionymi wartościami RPeaksAnnotatedSignalDatapoint.peak = true, jeżeli w danym punkcie występuje górka
    };
};
