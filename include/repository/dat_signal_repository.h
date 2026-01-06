#ifndef EKG_DAT_SIGNAL_REPOSITORY_H
#define EKG_DAT_SIGNAL_REPOSITORY_H

#include "abstract/signal_repository.h"
#include "../dto/validation_result.h"
#include <QString>

class DATSignalRepository : public ISignalRepository {
private:
    mutable ValidationResult last_validation_result_;

public:
    std::shared_ptr<SignalDataset> Load(const QString& filename) override;
    ValidationResult GetLastValidationResult() const { return last_validation_result_; }
};

#endif //EKG_DAT_SIGNAL_REPOSITORY_H

