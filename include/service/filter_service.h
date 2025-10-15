#ifndef EKG_FILTER_SERVICE_H
#define EKG_FILTER_SERVICE_H
#include <vector>

#include "../model/signal_datapoint.h"

// TODO(Ola Szota): ten serwis zawiera dwie klasy z implementacją:
// butterworth_filter_service.cpp
// moving_average_filter_service.cpp
// serwis zawiera jedną metodę Filter, która przyjmuje wektor z punktami danych
// Metoda powinna zwrócić przefiltrowany sygnał.
// Oczywiście długość zwracanego wektora powinna być taka sama jak długość wektora w argumencie.
class IFilterService {
public:
    virtual ~IFilterService() = default;

    virtual std::vector<SignalDatapoint> Filter(std::vector<SignalDatapoint> values) = 0;
};

#endif //EKG_FILTER_SERVICE_H
