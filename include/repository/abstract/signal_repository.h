#ifndef EKG_SIGNAL_REPOSITORY_H
#define EKG_SIGNAL_REPOSITORY_H
#include "../../model/signal_dataset.h"
#include <QString>

class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    virtual std::shared_ptr<SignalDataset> Load(const QString& source) = 0;
};

#endif //EKG_SIGNAL_REPOSITORY_H

