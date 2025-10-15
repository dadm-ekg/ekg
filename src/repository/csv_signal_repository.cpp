#include "../../include/repository/signal_repository.h"

class CSVSignalRepository : public ISignalRepository {
public:
    std::vector<SignalDatapoint> Load(QString filename) override {
        return {};
    }
};
