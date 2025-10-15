#ifndef EKG_SIGNAL_REPOSITORY_H
#define EKG_SIGNAL_REPOSITORY_H
#include <qstring.h>

#include "../model/signal_datapoint.h"

class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    std::vector<SignalDatapoint> Load(QString filename);
};

#endif //EKG_SIGNAL_REPOSITORY_H
