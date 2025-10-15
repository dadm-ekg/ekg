#ifndef EKG_SIGNAL_REPOSITORY_H
#define EKG_SIGNAL_REPOSITORY_H
#include <qstring.h>

#include "../model/signal_datapoint.h"

class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    virtual std::shared_ptr<std::vector<SignalDatapoint>> Load(QString filename) = 0;
};

#endif //EKG_SIGNAL_REPOSITORY_H
