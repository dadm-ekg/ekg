#ifndef EKG_APPLICATION_SERVICE_H
#define EKG_APPLICATION_SERVICE_H

#include <QString>

#include "../../dto/filter_method.h"
#include "../../dto/r_peaks_detection_method.h"
#include "../../model/signal_dataset.h"
#include "../../model/r_peaks_annotated_signal_datapoint.h"

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    virtual bool Load(const QString &filename) = 0;

    virtual std::shared_ptr<SignalDataset> GetData() const = 0;

    virtual std::shared_ptr<SignalDataset> GetFilteredData() const = 0;

    virtual bool IsFileLoaded() const = 0;

    virtual QString GetLoadedFilename() const = 0;

    virtual std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint> > GetRPeaks() const = 0;

    virtual bool RunFiltering(FilterMethod method) const = 0;

    virtual bool CalculateRPeaks(RPeaksDetectionMethod method) const = 0;
};

#endif //EKG_APPLICATION_SERVICE_H
