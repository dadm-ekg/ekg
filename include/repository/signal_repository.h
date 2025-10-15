#ifndef EKG_SIGNAL_REPOSITORY_H
#define EKG_SIGNAL_REPOSITORY_H
#include "../model/signal_dataset.h"

class ISignalRepository {
public:
    virtual ~ISignalRepository() = default;

    virtual std::shared_ptr<SignalDataset> Load(QString filename) = 0;
};

#endif //EKG_SIGNAL_REPOSITORY_H
