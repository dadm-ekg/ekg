#include <iostream>
#include <ostream>

#include "../../include/repository/signal_repository.h"
#include <qstring.h>

class CSVSignalRepository : public ISignalRepository {
public:
    std::shared_ptr<SignalDataset> Load(QString filename) override {
        std::cout << "CSVSignalRepository::Load " << filename.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }
};
