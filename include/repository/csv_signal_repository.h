#ifndef EKG_CSV_SIGNAL_REPOSITORY_H
#define EKG_CSV_SIGNAL_REPOSITORY_H

#include "abstract/signal_repository.h"
#include <QString>

class CSVSignalRepository : public ISignalRepository {
public:
    std::shared_ptr<SignalDataset> Load(const QString& filename) override;
};

#endif //EKG_CSV_SIGNAL_REPOSITORY_H

