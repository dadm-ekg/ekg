#include "../../include/service/r_peaks_detection_service.h"

std::vector<RPeaksAnnotatedSignalDatapoint> RPeaksDetectionService::Detect(
    const std::vector<SignalDatapoint> &datapoints, int frequency) {
    // TODO(Mati P.): trzeba uzupełnić. Ta metoda musi zwrócić wektor RPeaksAnnotatedSignalDatapoint z uzupełnionymi wartościami RPeaksAnnotatedSignalDatapoint.peak = true, jeżeli w danym punkcie występuje górka
    return std::vector<RPeaksAnnotatedSignalDatapoint>{};
}
