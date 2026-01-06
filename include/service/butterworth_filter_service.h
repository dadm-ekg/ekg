#ifndef EKG_BUTTERWORTH_FILTER_SERVICE_H
#define EKG_BUTTERWORTH_FILTER_SERVICE_H

#include "abstract/filter_service.h"

class ButterworthFilterService : public IFilterService {
public:
    std::vector<SignalDatapoint> Filter(const std::vector<SignalDatapoint>& values, int windowSize = 5, int polynomialOrder = 2) override;
};

#endif //EKG_BUTTERWORTH_FILTER_SERVICE_H

