#ifndef EKG_RESULTS_REPOSITORY_H
#define EKG_RESULTS_REPOSITORY_H

#include <QString>
#include "../../dto/file_format.h"
#include "../../dto/hrv_time_metrics.h"
#include "../../dto/hrv_geo_metrics.h"
#include "../../dto/hrv_dfa_metrics.h"
#include "../../dto/heart_class_result.h"
#include "../../model/signal_dataset.h"
#include "../../model/r_peaks_annotated_signal_datapoint.h"
#include "../../model/wave_annotated_signal_datapoint.h"

class IResultsRepository {
public:
    virtual ~IResultsRepository() = default;

    virtual bool ExportFilteredSignal(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<SignalDataset> filtered_data
    ) = 0;

    virtual bool ExportRPeaks(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks,
        std::shared_ptr<SignalDataset> filtered_data
    ) = 0;

    virtual bool ExportHRVTime(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HRVTimeMetrics &hrv_time_metrics
    ) = 0;

    virtual bool ExportHRVGeo(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HRVGeoMetrics &hrv_geo_metrics
    ) = 0;

    virtual bool ExportHRVDFA(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HRVDFAMetrics &hrv_dfa_metrics
    ) = 0;

    virtual bool ExportWaves(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves,
        std::shared_ptr<SignalDataset> filtered_data
    ) = 0;

    virtual bool ExportHeartClass(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HeartClassResult &heart_class_result,
        double sampling_frequency
    ) = 0;
};

#endif //EKG_RESULTS_REPOSITORY_H

