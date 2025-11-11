#ifndef EKG_HRV_DFA_PROCESSING_SERVICE_IMPL_H
#define EKG_HRV_DFA_PROCESSING_SERVICE_IMPL_H

#include "abstract/hrv_dfa_processing_service.h"

class HRVDFAProcessingService : public IHRVDFAProcessingService {
public:
    HRVDFAMetrics Process(const std::vector<SignalDatapoint>& datapoints, int frequency) override;
};

#endif //EKG_HRV_DFA_PROCESSING_SERVICE_IMPL_H
