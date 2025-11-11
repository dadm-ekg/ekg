#ifndef EKG_DAT_SIGNAL_REPOSITORY_H
#define EKG_DAT_SIGNAL_REPOSITORY_H

#include "abstract/signal_repository.h"
#include <QString>

class DATSignalRepository : public ISignalRepository {
public:
    std::shared_ptr<SignalDataset> Load(const QString& filename) override;
};

#endif //EKG_DAT_SIGNAL_REPOSITORY_H

