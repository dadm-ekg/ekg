#ifndef EKG_SAVITZKY_GOLAY_FILTER_SERVICE_H
#define EKG_SAVITZKY_GOLAY_FILTER_SERVICE_H

#include "abstract/filter_service.h"

class SavitzkyGolayFilterService : public IFilterService {
public:
    std::vector<SignalDatapoint> Filter(const std::vector<SignalDatapoint>& values, int windowSize = 5, int polynomialOrder = 2) override;

private:
    std::vector<double> computeCoefficients(int windowSize, int polynomialOrder);
};

#endif //EKG_SAVITZKY_GOLAY_FILTER_SERVICE_H

