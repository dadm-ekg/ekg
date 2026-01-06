#ifndef EKG_RESULTS_REPOSITORY_IMPL_H
#define EKG_RESULTS_REPOSITORY_IMPL_H

#include "abstract/results_repository.h"

class ResultsRepository : public IResultsRepository {
public:
    bool ExportFilteredSignal(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<SignalDataset> filtered_data
    ) override;

    bool ExportRPeaks(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks,
        std::shared_ptr<SignalDataset> filtered_data
    ) override;

    bool ExportHRVTime(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HRVTimeMetrics &hrv_time_metrics
    ) override;

    bool ExportHRVGeo(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HRVGeoMetrics &hrv_geo_metrics
    ) override;

    bool ExportWaves(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves,
        std::shared_ptr<SignalDataset> filtered_data
    ) override;

    bool ExportHeartClass(
        FileFormat format,
        const QString &filepath,
        const QString &filename,
        const HeartClassResult &heart_class_result,
        double sampling_frequency
    ) override;

private:
    bool ExportFilteredSignalJSON(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportFilteredSignalCSV(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportFilteredSignalHTML(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data);

    bool ExportRPeaksJSON(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportRPeaksCSV(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportRPeaksHTML(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data);

    bool ExportHRVTimeJSON(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics);
    bool ExportHRVTimeCSV(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics);
    bool ExportHRVTimeHTML(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics);

    bool ExportHRVGeoJSON(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics);
    bool ExportHRVGeoCSV(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics);
    bool ExportHRVGeoHTML(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics);

    bool ExportWavesJSON(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportWavesCSV(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data);
    bool ExportWavesHTML(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data);

    bool ExportHeartClassJSON(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result);
    bool ExportHeartClassCSV(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result, double sampling_frequency);
    bool ExportHeartClassHTML(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result, double sampling_frequency);
};

#endif //EKG_RESULTS_REPOSITORY_IMPL_H

