#ifndef EKG_CONTROLLER_H
#define EKG_CONTROLLER_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QThread>
#include <memory>
#include "../service/abstract/application_service.h"
#include "../repository/abstract/results_repository.h"
#include "../dto/filter_method.h"
#include "../dto/r_peaks_detection_method.h"
#include "../dto/hrv_spectral_method.h"

class AnalysisWorker;

class EkgController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString loadedFilename READ loadedFilename NOTIFY loadedFilenameChanged)
    Q_PROPERTY(bool isFileLoaded READ isFileLoaded NOTIFY isFileLoadedChanged)
    Q_PROPERTY(bool hasData READ hasData NOTIFY hasDataChanged)
    Q_PROPERTY(bool hasFilteredData READ hasFilteredData NOTIFY hasFilteredDataChanged)
    Q_PROPERTY(bool baselineCompleted READ baselineCompleted NOTIFY baselineCompletedChanged)
    Q_PROPERTY(bool rPeaksCompleted READ rPeaksCompleted NOTIFY rPeaksCompletedChanged)
    Q_PROPERTY(bool hrvTimeCompleted READ hrvTimeCompleted NOTIFY hrvTimeCompletedChanged)
    Q_PROPERTY(bool hrvGeoCompleted READ hrvGeoCompleted NOTIFY hrvGeoCompletedChanged)
    Q_PROPERTY(bool wavesCompleted READ wavesCompleted NOTIFY wavesCompletedChanged)
    Q_PROPERTY(bool heartClassCompleted READ heartClassCompleted NOTIFY heartClassCompletedChanged)

public:
    Q_ENUM(FilterMethod)
    Q_ENUM(RPeaksDetectionMethod)
    Q_ENUM(HRVSpectralMethod)

    explicit EkgController(std::shared_ptr<IApplicationService> application_service, std::shared_ptr<IResultsRepository> results_repository, QObject *parent = nullptr);

    Q_INVOKABLE void loadData(const QString &filename);
    Q_INVOKABLE void openFileDialog();
    Q_INVOKABLE bool runBaseline(int filterMethod, int windowSize = 5, int polynomialOrder = 2);
    Q_INVOKABLE bool runRPeaksDetection(int method);
    Q_INVOKABLE bool runHRVTime(int method);
    Q_INVOKABLE bool runHRVGeo();
    Q_INVOKABLE bool runWaves();
    Q_INVOKABLE bool runHeartClass();
    Q_INVOKABLE QStringList getAvailableFiles() const;
    Q_INVOKABLE void loadFileByName(const QString &filename);
    Q_INVOKABLE void resetBaseline();
    Q_INVOKABLE void resetRPeaks();
    Q_INVOKABLE void resetHRVTime();
    Q_INVOKABLE void resetHRVGeo();
    Q_INVOKABLE void resetWaves();
    Q_INVOKABLE void resetHeartClass();
    Q_INVOKABLE QVariantList getRawSeries(int channel = 0, int maxPoints = 4000, double startTime = -1, double endTime = -1) const;
    Q_INVOKABLE QVariantList getFilteredSeries(int channel = 0, int maxPoints = 4000, double startTime = -1, double endTime = -1) const;
    Q_INVOKABLE QVariantList getRPeakMarkers(int channel = 0, double startTime = -1, double endTime = -1) const;
    Q_INVOKABLE QVariantMap getHRVTimeMetrics() const;
    Q_INVOKABLE QVariantList getHRVTimePowerSpectrum() const;
    Q_INVOKABLE QVariantList getHRVTimeTachogram() const;
    Q_INVOKABLE QVariantMap getHRVGeoMetrics() const;
    Q_INVOKABLE QVariantList getHRVGeoHistogram() const;
    Q_INVOKABLE QVariantList getHRVGeoPoincare() const;
    Q_INVOKABLE QVariantMap getHeartClassBarChart() const;
    Q_INVOKABLE QVariantMap getWaveMarkers(int channel = 0) const;
    Q_INVOKABLE QVariantList getHeartClassAnnotations() const;
    Q_INVOKABLE int channelCount() const;
    Q_INVOKABLE double samplingFrequency() const;
    Q_INVOKABLE double signalDuration() const;
    Q_INVOKABLE bool exportFilteredSignal(int format, const QString &filepath);
    Q_INVOKABLE bool exportRPeaks(int format, const QString &filepath);
    Q_INVOKABLE bool exportHRVTime(int format, const QString &filepath);
    Q_INVOKABLE bool exportHRVGeo(int format, const QString &filepath);
    Q_INVOKABLE bool exportWaves(int format, const QString &filepath);
    Q_INVOKABLE bool exportHeartClass(int format, const QString &filepath);
    Q_INVOKABLE void openSaveSettingsDialog(int selectedFilterMethod, int selectedRPeaksMethod, int selectedHRVTimeMethod, int selectedChannelIndex, const QString &currentModule, bool isDarkTheme, double chartWindowSize, int maxPlottedPoints, int windowSize, int polynomialOrder);
    Q_INVOKABLE void openLoadSettingsDialog();

    QString loadedFilename() const;
    bool isFileLoaded() const;
    bool hasData() const;
    bool hasFilteredData() const;
    bool baselineCompleted() const;
    bool rPeaksCompleted() const;
    bool hrvTimeCompleted() const;
    bool hrvGeoCompleted() const;
    bool wavesCompleted() const;
    bool heartClassCompleted() const;

signals:
    void loadedFilenameChanged();
    void isFileLoadedChanged();
    void hasDataChanged();
    void hasFilteredDataChanged();
    void baselineCompletedChanged();
    void rPeaksCompletedChanged();
    void hrvTimeCompletedChanged();
    void hrvGeoCompletedChanged();
    void wavesCompletedChanged();
    void heartClassCompletedChanged();
    void fileLoadSuccess(const QString &filename);
    void fileLoadError(const QString &errorMessage);
    void filteringSuccess(const QString &filterName);
    void filteringError(const QString &errorMessage);
    void rPeaksDetectionSuccess(const QString &methodName);
    void rPeaksDetectionError(const QString &errorMessage);
    void hrvTimeSuccess(const QString &methodName);
    void hrvTimeError(const QString &errorMessage);
    void hrvGeoSuccess();
    void hrvGeoError(const QString &errorMessage);
    void wavesSuccess();
    void wavesError(const QString &errorMessage);
    void heartClassSuccess();
    void heartClassError(const QString &errorMessage);
    void settingsSaveSuccess(const QString &filepath);
    void settingsSaveError(const QString &errorMessage);
    void settingsLoadSuccess(const QVariantMap &settings);
    void settingsLoadError(const QString &errorMessage);

private:
    std::shared_ptr<IApplicationService> application_service_;
    std::shared_ptr<IResultsRepository> results_repository_;
    QThread* analysis_thread_;
    AnalysisWorker* analysis_worker_;
    bool baseline_completed_ = false;
    bool r_peaks_completed_ = false;
    bool hrv_time_completed_ = false;
    bool hrv_geo_completed_ = false;
    bool waves_completed_ = false;
    bool heart_class_completed_ = false;
    HRVTimeMetrics cached_hrv_metrics_;
    HRVGeoMetrics cached_hrv_geo_metrics_;
    HeartClassResult cached_heart_class_result_;
    
    void setupAnalysisWorker();
};

#endif
