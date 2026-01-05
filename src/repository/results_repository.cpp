#include "../../include/repository/results_repository.h"
#include "../../include/dto/file_format.h"
#include <QFile>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <algorithm>
#include <cmath>

bool ResultsRepository::ExportFilteredSignal(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    std::shared_ptr<SignalDataset> filtered_data
) {
    if (!filtered_data) return false;
    
    switch (format) {
        case FileFormat::CSV:
            return ExportFilteredSignalCSV(filepath, filename, filtered_data);
        case FileFormat::HTML:
            return ExportFilteredSignalHTML(filepath, filename, filtered_data);
        case FileFormat::JSON:
            return ExportFilteredSignalJSON(filepath, filename, filtered_data);
        default:
            return false;
    }
}

bool ResultsRepository::ExportRPeaks(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks,
    std::shared_ptr<SignalDataset> filtered_data
) {
    if (!r_peaks || !filtered_data) return false;
    
    switch (format) {
        case FileFormat::CSV:
            return ExportRPeaksCSV(filepath, filename, r_peaks, filtered_data);
        case FileFormat::HTML:
            return ExportRPeaksHTML(filepath, filename, r_peaks, filtered_data);
        case FileFormat::JSON:
            return ExportRPeaksJSON(filepath, filename, r_peaks, filtered_data);
        default:
            return false;
    }
}

bool ResultsRepository::ExportHRVTime(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    const HRVTimeMetrics &hrv_time_metrics
) {
    switch (format) {
        case FileFormat::CSV:
            return ExportHRVTimeCSV(filepath, filename, hrv_time_metrics);
        case FileFormat::HTML:
            return ExportHRVTimeHTML(filepath, filename, hrv_time_metrics);
        case FileFormat::JSON:
            return ExportHRVTimeJSON(filepath, filename, hrv_time_metrics);
        default:
            return false;
    }
}

bool ResultsRepository::ExportHRVGeo(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    const HRVGeoMetrics &hrv_geo_metrics
) {
    switch (format) {
        case FileFormat::CSV:
            return ExportHRVGeoCSV(filepath, filename, hrv_geo_metrics);
        case FileFormat::HTML:
            return ExportHRVGeoHTML(filepath, filename, hrv_geo_metrics);
        case FileFormat::JSON:
            return ExportHRVGeoJSON(filepath, filename, hrv_geo_metrics);
        default:
            return false;
    }
}

bool ResultsRepository::ExportWaves(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves,
    std::shared_ptr<SignalDataset> filtered_data
) {
    if (!waves || !filtered_data) return false;
    
    switch (format) {
        case FileFormat::CSV:
            return ExportWavesCSV(filepath, filename, waves, filtered_data);
        case FileFormat::HTML:
            return ExportWavesHTML(filepath, filename, waves, filtered_data);
        case FileFormat::JSON:
            return ExportWavesJSON(filepath, filename, waves, filtered_data);
        default:
            return false;
    }
}

bool ResultsRepository::ExportHeartClass(
    FileFormat format,
    const QString &filepath,
    const QString &filename,
    const HeartClassResult &heart_class_result,
    double sampling_frequency
) {
    switch (format) {
        case FileFormat::CSV:
            return ExportHeartClassCSV(filepath, filename, heart_class_result, sampling_frequency);
        case FileFormat::HTML:
            return ExportHeartClassHTML(filepath, filename, heart_class_result, sampling_frequency);
        case FileFormat::JSON:
            return ExportHeartClassJSON(filepath, filename, heart_class_result);
        default:
            return false;
    }
}

bool ResultsRepository::ExportFilteredSignalJSON(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "ECG BASELINE";
    root["filename"] = filename;
    
    QJsonObject data;
    data["filter_applied"] = true;
    data["sample_count"] = static_cast<int>(filtered_data->values.size());
    data["frequency"] = filtered_data->frequency;
    data["channels"] = static_cast<int>(filtered_data->values.empty() ? 0 : filtered_data->values.front().channelValues.size());
    
    QJsonArray signalData;
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    for (size_t i = 0; i < filtered_data->values.size(); ++i) {
        const auto &sample = filtered_data->values[i];
        QJsonObject sampleObj;
        sampleObj["sample"] = static_cast<int>(i);
        sampleObj["time"] = static_cast<double>(i) / frequency;
        QJsonArray values;
        for (const auto &val : sample.channelValues) {
            values.append(val);
        }
        sampleObj["values"] = values;
        signalData.append(sampleObj);
    }
    data["signal_data"] = signalData;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportFilteredSignalCSV(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,ECG BASELINE\n";
    out << "Filename," << filename << "\n";
    out << "Frequency," << filtered_data->frequency << " Hz\n";
    out << "Sample Count," << filtered_data->values.size() << "\n";
    out << "Channels," << (filtered_data->values.empty() ? 0 : filtered_data->values.front().channelValues.size()) << "\n";
    out << "\n";
    out << "Sample,Time";
    if (!filtered_data->values.empty()) {
        for (size_t ch = 0; ch < filtered_data->values.front().channelValues.size(); ++ch) {
            out << ",Channel " << (ch + 1);
        }
    }
    out << "\n";
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    for (size_t i = 0; i < filtered_data->values.size(); ++i) {
        const auto &sample = filtered_data->values[i];
        out << static_cast<int>(i) << "," << (static_cast<double>(i) / frequency);
        for (const auto &val : sample.channelValues) {
            out << "," << val;
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportFilteredSignalHTML(const QString &filepath, const QString &filename, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    const size_t maxSamples = std::min(filtered_data->values.size(), static_cast<size_t>(50000));
    const size_t numChannels = filtered_data->values.empty() ? 0 : filtered_data->values.front().channelValues.size();
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - ECG BASELINE</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "h3 { color: #666; margin-top: 20px; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 4px; }\n";
    out << "canvas { max-height: 400px; }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> ECG BASELINE</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "<p><strong>Frequency:</strong> " << filtered_data->frequency << " Hz</p>\n";
    out << "<p><strong>Sample Count:</strong> " << filtered_data->values.size() << "</p>\n";
    out << "<p><strong>Channels:</strong> " << numChannels << "</p>\n";
    if (maxSamples < filtered_data->values.size()) {
        out << "<p><em>Note: Showing first " << maxSamples << " samples for performance</em></p>\n";
    }
    out << "</div>\n";
    out << "<h2>Baseline Filtering Results</h2>\n";
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "<div class=\"chart-container\">\n";
        out << "<h3>Channel " << (ch + 1) << "</h3>\n";
        out << "<canvas id=\"chart" << ch << "\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    out << "const frequency = " << frequency << ";\n";
    out << "const maxSamples = " << maxSamples << ";\n";
    out << "const numChannels = " << numChannels << ";\n";
    out << "const chartData = [];\n";
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "chartData[" << ch << "] = { labels: [], datasets: [{ label: 'Channel " << (ch + 1) << "', data: [], borderColor: 'rgb(75, 192, 192)', backgroundColor: 'rgba(75, 192, 192, 0.2)', borderWidth: 1, pointRadius: 0 }] };\n";
    }
    
    for (size_t i = 0; i < maxSamples; ++i) {
        const auto &sample = filtered_data->values[i];
        const double time = static_cast<double>(i) / frequency;
        out << "const time" << i << " = " << time << ";\n";
        for (size_t ch = 0; ch < numChannels && ch < sample.channelValues.size(); ++ch) {
            out << "chartData[" << ch << "].labels.push(time" << i << ");\n";
            out << "chartData[" << ch << "].datasets[0].data.push(" << sample.channelValues[ch] << ");\n";
        }
    }
    
    out << "for (let ch = 0; ch < numChannels; ch++) {\n";
    out << "  const ctx = document.getElementById('chart' + ch);\n";
    out << "  new Chart(ctx, {\n";
    out << "    type: 'line',\n";
    out << "    data: chartData[ch],\n";
    out << "    options: {\n";
    out << "      responsive: true,\n";
    out << "      maintainAspectRatio: true,\n";
    out << "      scales: {\n";
    out << "        x: { title: { display: true, text: 'Time (s)' } },\n";
    out << "        y: { title: { display: true, text: 'Amplitude' } }\n";
    out << "      },\n";
    out << "      plugins: {\n";
    out << "        legend: { display: true },\n";
    out << "        tooltip: { mode: 'index', intersect: false }\n";
    out << "      }\n";
    out << "    }\n";
    out << "  });\n";
    out << "}\n";
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}

bool ResultsRepository::ExportRPeaksJSON(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "R PEAKS";
    root["filename"] = filename;
    
    QJsonObject data;
    data["frequency"] = filtered_data->frequency;
    data["sample_count"] = static_cast<int>(r_peaks->size());
    data["channels"] = static_cast<int>(r_peaks->empty() ? 0 : r_peaks->front().channelValues.size());
    
    QJsonArray signalData;
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    std::vector<int> peakCounts(r_peaks->empty() ? 0 : r_peaks->front().peaks.size(), 0);
    for (size_t i = 0; i < r_peaks->size(); ++i) {
        const auto &peak = (*r_peaks)[i];
        QJsonObject sampleObj;
        sampleObj["sample"] = static_cast<int>(i);
        sampleObj["time"] = static_cast<double>(i) / frequency;
        QJsonArray values;
        for (const auto &val : peak.channelValues) {
            values.append(val);
        }
        sampleObj["values"] = values;
        QJsonArray peaksArray;
        for (size_t ch = 0; ch < peak.peaks.size(); ++ch) {
            bool isPeak = peak.peaks[ch];
            peaksArray.append(isPeak);
            if (isPeak) peakCounts[ch]++;
        }
        sampleObj["peaks"] = peaksArray;
        signalData.append(sampleObj);
    }
    QJsonArray peakCountsArray;
    for (int count : peakCounts) {
        peakCountsArray.append(count);
    }
    data["peak_counts"] = peakCountsArray;
    data["signal_data"] = signalData;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportRPeaksCSV(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,R PEAKS\n";
    out << "Filename," << filename << "\n";
    out << "Frequency," << filtered_data->frequency << " Hz\n";
    out << "Sample Count," << r_peaks->size() << "\n";
    out << "\n";
    out << "Sample,Time";
    if (!r_peaks->empty()) {
        for (size_t ch = 0; ch < r_peaks->front().channelValues.size(); ++ch) {
            out << ",Channel " << (ch + 1);
        }
        for (size_t ch = 0; ch < r_peaks->front().peaks.size(); ++ch) {
            out << ",Is_Peak_Channel_" << (ch + 1);
        }
    }
    out << "\n";
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    for (size_t i = 0; i < r_peaks->size(); ++i) {
        const auto &peak = (*r_peaks)[i];
        out << static_cast<int>(i) << "," << (static_cast<double>(i) / frequency);
        for (const auto &val : peak.channelValues) {
            out << "," << val;
        }
        for (bool isPeak : peak.peaks) {
            out << "," << (isPeak ? "1" : "0");
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportRPeaksHTML(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<RPeaksAnnotatedSignalDatapoint>> r_peaks, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    const size_t maxSamples = std::min(r_peaks->size(), static_cast<size_t>(50000));
    const size_t numChannels = r_peaks->empty() ? 0 : r_peaks->front().channelValues.size();
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - R PEAKS</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "h3 { color: #666; margin-top: 20px; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 4px; }\n";
    out << "canvas { max-height: 400px; }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> R PEAKS</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "<p><strong>Frequency:</strong> " << filtered_data->frequency << " Hz</p>\n";
    out << "<p><strong>Sample Count:</strong> " << r_peaks->size() << "</p>\n";
    if (maxSamples < r_peaks->size()) {
        out << "<p><em>Note: Showing first " << maxSamples << " samples for performance</em></p>\n";
    }
    out << "</div>\n";
    out << "<h2>R Peaks Detection Results</h2>\n";
    
    if (!r_peaks->empty()) {
        out << "<p><strong>R-peaks detected per channel:</strong></p>\n";
        out << "<ul>\n";
        for (size_t ch = 0; ch < r_peaks->front().peaks.size(); ++ch) {
            int peakCount = 0;
            for (const auto &peak : *r_peaks) {
                if (ch < peak.peaks.size() && peak.peaks[ch]) peakCount++;
            }
            out << "<li>Channel " << (ch + 1) << ": " << peakCount << " peaks</li>\n";
        }
        out << "</ul>\n";
    }
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "<div class=\"chart-container\">\n";
        out << "<h3>Channel " << (ch + 1) << "</h3>\n";
        out << "<canvas id=\"chart" << ch << "\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    out << "const frequency = " << frequency << ";\n";
    out << "const maxSamples = " << maxSamples << ";\n";
    out << "const numChannels = " << numChannels << ";\n";
    out << "const chartData = [];\n";
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "chartData[" << ch << "] = { labels: [], datasets: [\n";
        out << "  { type: 'line', label: 'Signal', data: [], borderColor: 'rgb(75, 192, 192)', backgroundColor: 'rgba(75, 192, 192, 0.2)', borderWidth: 1, pointRadius: 0 },\n";
        out << "  { type: 'scatter', label: 'R Peaks', data: [], borderColor: 'rgb(255, 99, 132)', backgroundColor: 'rgb(255, 99, 132)', pointRadius: 5, pointHoverRadius: 7 }\n";
        out << "] };\n";
    }
    
    for (size_t i = 0; i < maxSamples; ++i) {
        const auto &peak = (*r_peaks)[i];
        const double time = static_cast<double>(i) / frequency;
        for (size_t ch = 0; ch < numChannels && ch < peak.channelValues.size(); ++ch) {
            out << "chartData[" << ch << "].labels.push(" << time << ");\n";
            out << "chartData[" << ch << "].datasets[0].data.push(" << peak.channelValues[ch] << ");\n";
        }
    }
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < maxSamples; ++i) {
            const auto &peak = (*r_peaks)[i];
            if (ch < peak.peaks.size() && peak.peaks[ch] && ch < peak.channelValues.size()) {
                const double time = static_cast<double>(i) / frequency;
                out << "chartData[" << ch << "].datasets[1].data.push({x: " << time << ", y: " << peak.channelValues[ch] << "});\n";
            }
        }
    }
    
    out << "for (let ch = 0; ch < numChannels; ch++) {\n";
    out << "  const ctx = document.getElementById('chart' + ch);\n";
    out << "  new Chart(ctx, {\n";
    out << "    data: chartData[ch],\n";
    out << "    options: {\n";
    out << "      responsive: true,\n";
    out << "      maintainAspectRatio: true,\n";
    out << "      scales: {\n";
    out << "        x: { type: 'linear', title: { display: true, text: 'Time (s)' } },\n";
    out << "        y: { title: { display: true, text: 'Amplitude' } }\n";
    out << "      },\n";
    out << "      plugins: {\n";
    out << "        legend: { display: true },\n";
    out << "        tooltip: { mode: 'nearest', intersect: false }\n";
    out << "      }\n";
    out << "    }\n";
    out << "  });\n";
    out << "}\n";
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}

bool ResultsRepository::ExportHRVTimeJSON(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "HRV TIME";
    root["filename"] = filename;
    
    QJsonObject data;
    data["rr_mean"] = hrv_time_metrics.rr_mean;
    data["sdnn"] = hrv_time_metrics.sdnn;
    data["rmssd"] = hrv_time_metrics.rmssd;
    data["nn50"] = hrv_time_metrics.nn50;
    data["pnn50"] = hrv_time_metrics.pnn50;
    data["tp"] = hrv_time_metrics.tp;
    data["vlf"] = hrv_time_metrics.vlf;
    data["lf"] = hrv_time_metrics.lf;
    data["hf"] = hrv_time_metrics.hf;
    data["lf_hf_ratio"] = hrv_time_metrics.lf_hf;
    QJsonArray powerSpectrum;
    for (const auto &val : hrv_time_metrics.power_spectrum) {
        powerSpectrum.append(val);
    }
    data["power_spectrum"] = powerSpectrum;
    QJsonArray frequencies;
    for (const auto &val : hrv_time_metrics.frequencies) {
        frequencies.append(val);
    }
    data["frequencies"] = frequencies;
    QJsonArray rrIntervals;
    for (const auto &val : hrv_time_metrics.rr_intervals) {
        rrIntervals.append(val);
    }
    data["rr_intervals"] = rrIntervals;
    QJsonArray tachogramTimes;
    for (const auto &val : hrv_time_metrics.tachogram_times) {
        tachogramTimes.append(val);
    }
    data["tachogram_times"] = tachogramTimes;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportHRVTimeCSV(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,HRV TIME\n";
    out << "Filename," << filename << "\n";
    out << "\n";
    out << "Parameter,Value\n";
    out << "RR_mean," << hrv_time_metrics.rr_mean << "\n";
    out << "SDNN," << hrv_time_metrics.sdnn << "\n";
    out << "RMSSD," << hrv_time_metrics.rmssd << "\n";
    out << "NN50," << hrv_time_metrics.nn50 << "\n";
    out << "pNN50," << hrv_time_metrics.pnn50 << "\n";
    out << "TP," << hrv_time_metrics.tp << "\n";
    out << "VLF," << hrv_time_metrics.vlf << "\n";
    out << "LF," << hrv_time_metrics.lf << "\n";
    out << "HF," << hrv_time_metrics.hf << "\n";
    out << "LF/HF," << hrv_time_metrics.lf_hf << "\n";
    out << "\n";
    out << "Frequency,Power\n";
    for (size_t i = 0; i < hrv_time_metrics.frequencies.size() && i < hrv_time_metrics.power_spectrum.size(); ++i) {
        out << hrv_time_metrics.frequencies[i] << "," << hrv_time_metrics.power_spectrum[i] << "\n";
    }
    out << "\n";
    out << "Time,RR_Interval\n";
    for (size_t i = 0; i < hrv_time_metrics.tachogram_times.size() && i < hrv_time_metrics.rr_intervals.size(); ++i) {
        out << hrv_time_metrics.tachogram_times[i] << "," << hrv_time_metrics.rr_intervals[i] << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportHRVTimeHTML(const QString &filepath, const QString &filename, const HRVTimeMetrics &hrv_time_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - HRV TIME</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "table { border-collapse: collapse; width: 100%; margin: 20px 0; background-color: white; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    out << "th { background-color: #4CAF50; color: white; font-weight: bold; }\n";
    out << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
    out << "tr:hover { background-color: #f5f5f5; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> HRV TIME</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "</div>\n";
    out << "<h2>HRV Time Domain Metrics</h2>\n";
    out << "<table>\n";
    out << "<tr><th>Parameter</th><th>Value</th></tr>\n";
    out << "<tr><td>RR_mean</td><td>" << hrv_time_metrics.rr_mean << " ms</td></tr>\n";
    out << "<tr><td>SDNN</td><td>" << hrv_time_metrics.sdnn << " ms</td></tr>\n";
    out << "<tr><td>RMSSD</td><td>" << hrv_time_metrics.rmssd << " ms</td></tr>\n";
    out << "<tr><td>NN50</td><td>" << hrv_time_metrics.nn50 << "</td></tr>\n";
    out << "<tr><td>pNN50</td><td>" << hrv_time_metrics.pnn50 << " %</td></tr>\n";
    out << "</table>\n";
    out << "<h2>HRV Frequency Domain Metrics</h2>\n";
    out << "<table>\n";
    out << "<tr><th>Parameter</th><th>Value</th></tr>\n";
    out << "<tr><td>TP</td><td>" << hrv_time_metrics.tp << " ms²</td></tr>\n";
    out << "<tr><td>VLF</td><td>" << hrv_time_metrics.vlf << " ms²</td></tr>\n";
    out << "<tr><td>LF</td><td>" << hrv_time_metrics.lf << " ms²</td></tr>\n";
    out << "<tr><td>HF</td><td>" << hrv_time_metrics.hf << " ms²</td></tr>\n";
    out << "<tr><td>LF/HF</td><td>" << hrv_time_metrics.lf_hf << "</td></tr>\n";
    out << "</table>\n";
    
    // Power Spectrum Chart
    if (!hrv_time_metrics.frequencies.empty() && !hrv_time_metrics.power_spectrum.empty()) {
        out << "<h2>Power Spectrum</h2>\n";
        out << "<div class=\"chart-container\">\n";
        out << "<canvas id=\"powerSpectrumChart\"></canvas>\n";
        out << "</div>\n";
    }
    
    // Tachogram Chart
    if (!hrv_time_metrics.tachogram_times.empty() && !hrv_time_metrics.rr_intervals.empty()) {
        out << "<h2>RR Interval Tachogram</h2>\n";
        out << "<div class=\"chart-container\">\n";
        out << "<canvas id=\"tachogramChart\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    
    // Power Spectrum Chart
    if (!hrv_time_metrics.frequencies.empty() && !hrv_time_metrics.power_spectrum.empty()) {
        out << "const powerSpectrumCtx = document.getElementById('powerSpectrumChart').getContext('2d');\n";
        out << "const powerSpectrumData = {\n";
        out << "  labels: [";
        for (size_t i = 0; i < hrv_time_metrics.frequencies.size(); ++i) {
            if (i > 0) out << ", ";
            out << hrv_time_metrics.frequencies[i];
        }
        out << "],\n";
        out << "  datasets: [{\n";
        out << "    label: 'Power Spectrum',\n";
        out << "    data: [";
        for (size_t i = 0; i < hrv_time_metrics.power_spectrum.size(); ++i) {
            if (i > 0) out << ", ";
            out << hrv_time_metrics.power_spectrum[i];
        }
        out << "],\n";
        out << "    borderColor: 'rgb(75, 192, 192)',\n";
        out << "    backgroundColor: 'rgba(75, 192, 192, 0.2)',\n";
        out << "    tension: 0.1\n";
        out << "  }]\n";
        out << "};\n";
        out << "new Chart(powerSpectrumCtx, {\n";
        out << "  type: 'line',\n";
        out << "  data: powerSpectrumData,\n";
        out << "  options: {\n";
        out << "    responsive: true,\n";
        out << "    plugins: { legend: { display: true } },\n";
        out << "    scales: {\n";
        out << "      x: { title: { display: true, text: 'Frequency (Hz)' } },\n";
        out << "      y: { title: { display: true, text: 'Power (ms²)' } }\n";
        out << "    }\n";
        out << "  }\n";
        out << "});\n";
    }
    
    // Tachogram Chart
    if (!hrv_time_metrics.tachogram_times.empty() && !hrv_time_metrics.rr_intervals.empty()) {
        out << "const tachogramCtx = document.getElementById('tachogramChart').getContext('2d');\n";
        out << "const tachogramData = {\n";
        out << "  labels: [";
        for (size_t i = 0; i < hrv_time_metrics.tachogram_times.size(); ++i) {
            if (i > 0) out << ", ";
            out << hrv_time_metrics.tachogram_times[i];
        }
        out << "],\n";
        out << "  datasets: [{\n";
        out << "    label: 'RR Intervals',\n";
        out << "    data: [";
        for (size_t i = 0; i < hrv_time_metrics.rr_intervals.size(); ++i) {
            if (i > 0) out << ", ";
            out << hrv_time_metrics.rr_intervals[i];
        }
        out << "],\n";
        out << "    borderColor: 'rgb(255, 99, 132)',\n";
        out << "    backgroundColor: 'rgba(255, 99, 132, 0.2)',\n";
        out << "    tension: 0.1\n";
        out << "  }]\n";
        out << "};\n";
        out << "new Chart(tachogramCtx, {\n";
        out << "  type: 'line',\n";
        out << "  data: tachogramData,\n";
        out << "  options: {\n";
        out << "    responsive: true,\n";
        out << "    plugins: { legend: { display: true } },\n";
        out << "    scales: {\n";
        out << "      x: { title: { display: true, text: 'Time (s)' } },\n";
        out << "      y: { title: { display: true, text: 'RR Interval (ms)' } }\n";
        out << "    }\n";
        out << "  }\n";
        out << "});\n";
    }
    
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}

bool ResultsRepository::ExportHRVGeoJSON(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "HRV GEO";
    root["filename"] = filename;
    
    QJsonObject data;
    data["triangular_index"] = hrv_geo_metrics.triangular_index;
    data["tinn"] = hrv_geo_metrics.tinn;
    data["sd1"] = hrv_geo_metrics.sd1;
    data["sd2"] = hrv_geo_metrics.sd2;
    QJsonArray histogram;
    for (const auto &val : hrv_geo_metrics.histogram) {
        histogram.append(val);
    }
    data["histogram"] = histogram;
    QJsonArray poincare;
    const auto& rr = hrv_geo_metrics.rr_intervals;
    for (size_t i = 0; i + 1 < rr.size(); ++i) {
        QJsonObject pointObj;
        pointObj["x"] = rr[i];
        pointObj["y"] = rr[i + 1];
        poincare.append(pointObj);
    }
    data["poincare_plot"] = poincare;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportHRVGeoCSV(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,HRV GEO\n";
    out << "Filename," << filename << "\n";
    out << "\n";
    out << "Parameter,Value\n";
    out << "Triangular Index," << hrv_geo_metrics.triangular_index << "\n";
    out << "TINN," << hrv_geo_metrics.tinn << "\n";
    out << "SD1," << hrv_geo_metrics.sd1 << "\n";
    out << "SD2," << hrv_geo_metrics.sd2 << "\n";
    out << "\n";
    out << "RR_Interval,Count\n";
    for (size_t i = 0; i < hrv_geo_metrics.histogram.size(); ++i) {
        out << i << "," << hrv_geo_metrics.histogram[i] << "\n";
    }
    out << "\n";
    out << "RR_n,RR_n+1\n";
    const auto& rr = hrv_geo_metrics.rr_intervals;
    for (size_t i = 0; i + 1 < rr.size(); ++i) {
        out << rr[i] << "," << rr[i + 1] << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportHRVGeoHTML(const QString &filepath, const QString &filename, const HRVGeoMetrics &hrv_geo_metrics) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - HRV GEO</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "table { border-collapse: collapse; width: 100%; margin: 20px 0; background-color: white; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    out << "th { background-color: #4CAF50; color: white; font-weight: bold; }\n";
    out << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
    out << "tr:hover { background-color: #f5f5f5; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> HRV GEO</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "</div>\n";
    out << "<h2>HRV Geometric Domain Metrics</h2>\n";
    out << "<table>\n";
    out << "<tr><th>Parameter</th><th>Value</th></tr>\n";
    out << "<tr><td>Triangular Index</td><td>" << hrv_geo_metrics.triangular_index << "</td></tr>\n";
    out << "<tr><td>TINN</td><td>" << hrv_geo_metrics.tinn << " ms</td></tr>\n";
    out << "<tr><td>SD1</td><td>" << hrv_geo_metrics.sd1 << " ms</td></tr>\n";
    out << "<tr><td>SD2</td><td>" << hrv_geo_metrics.sd2 << " ms</td></tr>\n";
    out << "</table>\n";
    
    // Histogram Chart
    if (!hrv_geo_metrics.histogram.empty()) {
        out << "<h2>RR Interval Histogram</h2>\n";
        out << "<div class=\"chart-container\">\n";
        out << "<canvas id=\"histogramChart\"></canvas>\n";
        out << "</div>\n";
    }
    
    // Poincaré Plot
    if (hrv_geo_metrics.rr_intervals.size() > 1) {
        out << "<h2>Poincaré Plot</h2>\n";
        out << "<div class=\"chart-container\">\n";
        out << "<canvas id=\"poincareChart\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    
    // Histogram Chart
    if (!hrv_geo_metrics.histogram.empty()) {
        out << "const histogramCtx = document.getElementById('histogramChart').getContext('2d');\n";
        out << "const histogramLabels = [";
        for (size_t i = 0; i < hrv_geo_metrics.histogram.size(); ++i) {
            if (i > 0) out << ", ";
            double binCenter = hrv_geo_metrics.rr_min + (i + 0.5) * hrv_geo_metrics.bin_width;
            out << binCenter;
        }
        out << "];\n";
        out << "const histogramData = {\n";
        out << "  labels: histogramLabels,\n";
        out << "  datasets: [{\n";
        out << "    label: 'Frequency',\n";
        out << "    data: [";
        for (size_t i = 0; i < hrv_geo_metrics.histogram.size(); ++i) {
            if (i > 0) out << ", ";
            out << hrv_geo_metrics.histogram[i];
        }
        out << "],\n";
        out << "    backgroundColor: 'rgba(54, 162, 235, 0.5)',\n";
        out << "    borderColor: 'rgba(54, 162, 235, 1)',\n";
        out << "    borderWidth: 1\n";
        out << "  }]\n";
        out << "};\n";
        out << "new Chart(histogramCtx, {\n";
        out << "  type: 'bar',\n";
        out << "  data: histogramData,\n";
        out << "  options: {\n";
        out << "    responsive: true,\n";
        out << "    plugins: { legend: { display: true } },\n";
        out << "    scales: {\n";
        out << "      x: { title: { display: true, text: 'RR Interval (ms)' } },\n";
        out << "      y: { title: { display: true, text: 'Frequency' }, beginAtZero: true }\n";
        out << "    }\n";
        out << "  }\n";
        out << "});\n";
    }
    
    // Poincaré Plot
    if (hrv_geo_metrics.rr_intervals.size() > 1) {
        out << "const poincareCtx = document.getElementById('poincareChart').getContext('2d');\n";
        out << "const poincareData = {\n";
        out << "  datasets: [{\n";
        out << "    label: 'Poincaré Plot',\n";
        out << "    data: [";
        for (size_t i = 0; i + 1 < hrv_geo_metrics.rr_intervals.size(); ++i) {
            if (i > 0) out << ", ";
            out << "{x: " << hrv_geo_metrics.rr_intervals[i] << ", y: " << hrv_geo_metrics.rr_intervals[i + 1] << "}";
        }
        out << "],\n";
        out << "    backgroundColor: 'rgba(255, 99, 132, 0.5)',\n";
        out << "    borderColor: 'rgba(255, 99, 132, 1)',\n";
        out << "    pointRadius: 2\n";
        out << "  }]\n";
        out << "};\n";
        out << "new Chart(poincareCtx, {\n";
        out << "  type: 'scatter',\n";
        out << "  data: poincareData,\n";
        out << "  options: {\n";
        out << "    responsive: true,\n";
        out << "    plugins: { legend: { display: true } },\n";
        out << "    scales: {\n";
        out << "      x: { title: { display: true, text: 'RR[n] (ms)' } },\n";
        out << "      y: { title: { display: true, text: 'RR[n+1] (ms)' } }\n";
        out << "    }\n";
        out << "  }\n";
        out << "});\n";
    }
    
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}

bool ResultsRepository::ExportWavesJSON(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "WAVES";
    root["filename"] = filename;
    
    QJsonObject data;
    data["frequency"] = filtered_data->frequency;
    data["sample_count"] = static_cast<int>(waves->size());
    data["channels"] = static_cast<int>(waves->empty() ? 0 : waves->front().channelValues.size());
    
    QJsonArray signalData;
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    for (size_t i = 0; i < waves->size(); ++i) {
        const auto &wave = (*waves)[i];
        QJsonObject sampleObj;
        sampleObj["sample"] = static_cast<int>(i);
        sampleObj["time"] = static_cast<double>(i) / frequency;
        QJsonArray values;
        for (const auto &val : wave.channelValues) {
            values.append(val);
        }
        sampleObj["values"] = values;
        
        QJsonArray pOnsetArray, pEndArray, qrsOnsetArray, qrsEndArray, tEndArray;
        for (size_t ch = 0; ch < wave.p_wave_onset.size(); ++ch) {
            bool pOnset = wave.p_wave_onset[ch];
            bool pEnd = ch < wave.p_wave_end.size() ? wave.p_wave_end[ch] : false;
            bool qrsOnset = ch < wave.qrs_onset.size() ? wave.qrs_onset[ch] : false;
            bool qrsEnd = ch < wave.qrs_end.size() ? wave.qrs_end[ch] : false;
            bool tEnd = ch < wave.t_end.size() ? wave.t_end[ch] : false;
            pOnsetArray.append(pOnset);
            pEndArray.append(pEnd);
            qrsOnsetArray.append(qrsOnset);
            qrsEndArray.append(qrsEnd);
            tEndArray.append(tEnd);
        }
        sampleObj["p_wave_onset"] = pOnsetArray;
        sampleObj["p_wave_end"] = pEndArray;
        sampleObj["qrs_onset"] = qrsOnsetArray;
        sampleObj["qrs_end"] = qrsEndArray;
        sampleObj["t_end"] = tEndArray;
        signalData.append(sampleObj);
    }
    data["signal_data"] = signalData;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportWavesCSV(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,WAVES\n";
    out << "Filename," << filename << "\n";
    out << "Frequency," << filtered_data->frequency << " Hz\n";
    out << "Sample Count," << waves->size() << "\n";
    out << "\n";
    out << "Sample,Time";
    if (!waves->empty()) {
        for (size_t ch = 0; ch < waves->front().channelValues.size(); ++ch) {
            out << ",Channel " << (ch + 1);
        }
        for (size_t ch = 0; ch < waves->front().p_wave_onset.size(); ++ch) {
            out << ",P_Onset_Ch" << (ch + 1) << ",P_End_Ch" << (ch + 1)
                << ",QRS_Onset_Ch" << (ch + 1) << ",QRS_End_Ch" << (ch + 1)
                << ",T_End_Ch" << (ch + 1);
        }
    }
    out << "\n";
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    for (size_t i = 0; i < waves->size(); ++i) {
        const auto &wave = (*waves)[i];
        out << static_cast<int>(i) << "," << (static_cast<double>(i) / frequency);
        for (const auto &val : wave.channelValues) {
            out << "," << val;
        }
        for (size_t ch = 0; ch < wave.p_wave_onset.size(); ++ch) {
            bool pOnset = wave.p_wave_onset[ch];
            bool pEnd = ch < wave.p_wave_end.size() ? wave.p_wave_end[ch] : false;
            bool qrsOnset = ch < wave.qrs_onset.size() ? wave.qrs_onset[ch] : false;
            bool qrsEnd = ch < wave.qrs_end.size() ? wave.qrs_end[ch] : false;
            bool tEnd = ch < wave.t_end.size() ? wave.t_end[ch] : false;
            out << "," << (pOnset ? "1" : "0");
            out << "," << (pEnd ? "1" : "0");
            out << "," << (qrsOnset ? "1" : "0");
            out << "," << (qrsEnd ? "1" : "0");
            out << "," << (tEnd ? "1" : "0");
        }
        out << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportWavesHTML(const QString &filepath, const QString &filename, std::shared_ptr<std::vector<WaveAnnotatedSignalDatapoint>> waves, std::shared_ptr<SignalDataset> filtered_data) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    const double frequency = filtered_data->frequency > 0 ? static_cast<double>(filtered_data->frequency) : 1.0;
    const size_t maxSamples = std::min(waves->size(), static_cast<size_t>(50000));
    const size_t numChannels = waves->empty() ? 0 : waves->front().channelValues.size();
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - WAVES</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "h3 { color: #666; margin-top: 20px; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-radius: 4px; }\n";
    out << "canvas { max-height: 400px; }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> WAVES</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "<p><strong>Frequency:</strong> " << filtered_data->frequency << " Hz</p>\n";
    out << "<p><strong>Sample Count:</strong> " << waves->size() << "</p>\n";
    if (maxSamples < waves->size()) {
        out << "<p><em>Note: Showing first " << maxSamples << " samples for performance</em></p>\n";
    }
    out << "</div>\n";
    out << "<h2>Wave Detection Summary</h2>\n";
    if (!waves->empty()) {
        out << "<p><strong>Wave markers detected per channel:</strong></p>\n";
        out << "<ul>\n";
        for (size_t ch = 0; ch < waves->front().p_wave_onset.size(); ++ch) {
            int pOnsetCount = 0, pEndCount = 0, qrsOnsetCount = 0, qrsEndCount = 0, tEndCount = 0;
            for (const auto &wave : *waves) {
                if (ch < wave.p_wave_onset.size() && wave.p_wave_onset[ch]) pOnsetCount++;
                if (ch < wave.p_wave_end.size() && wave.p_wave_end[ch]) pEndCount++;
                if (ch < wave.qrs_onset.size() && wave.qrs_onset[ch]) qrsOnsetCount++;
                if (ch < wave.qrs_end.size() && wave.qrs_end[ch]) qrsEndCount++;
                if (ch < wave.t_end.size() && wave.t_end[ch]) tEndCount++;
            }
            out << "<li>Channel " << (ch + 1) << ": P Onset=" << pOnsetCount << ", P End=" << pEndCount << ", QRS Onset=" << qrsOnsetCount << ", QRS End=" << qrsEndCount << ", T End=" << tEndCount << "</li>\n";
        }
        out << "</ul>\n";
    }
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "<div class=\"chart-container\">\n";
        out << "<h3>Channel " << (ch + 1) << "</h3>\n";
        out << "<canvas id=\"chart" << ch << "\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    out << "const frequency = " << frequency << ";\n";
    out << "const maxSamples = " << maxSamples << ";\n";
    out << "const numChannels = " << numChannels << ";\n";
    out << "const chartData = [];\n";
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        out << "chartData[" << ch << "] = { labels: [], datasets: [\n";
        out << "  { type: 'line', label: 'Signal', data: [], borderColor: 'rgb(75, 192, 192)', backgroundColor: 'rgba(75, 192, 192, 0.2)', borderWidth: 1, pointRadius: 0 },\n";
        out << "  { type: 'scatter', label: 'P Onset', data: [], borderColor: 'rgb(255, 99, 132)', backgroundColor: 'rgb(255, 99, 132)', pointRadius: 4, pointHoverRadius: 6 },\n";
        out << "  { type: 'scatter', label: 'P End', data: [], borderColor: 'rgb(54, 162, 235)', backgroundColor: 'rgb(54, 162, 235)', pointRadius: 4, pointHoverRadius: 6 },\n";
        out << "  { type: 'scatter', label: 'QRS Onset', data: [], borderColor: 'rgb(255, 206, 86)', backgroundColor: 'rgb(255, 206, 86)', pointRadius: 4, pointHoverRadius: 6 },\n";
        out << "  { type: 'scatter', label: 'QRS End', data: [], borderColor: 'rgb(75, 192, 192)', backgroundColor: 'rgb(75, 192, 192)', pointRadius: 4, pointHoverRadius: 6 },\n";
        out << "  { type: 'scatter', label: 'T End', data: [], borderColor: 'rgb(153, 102, 255)', backgroundColor: 'rgb(153, 102, 255)', pointRadius: 4, pointHoverRadius: 6 }\n";
        out << "] };\n";
    }
    
    for (size_t i = 0; i < maxSamples; ++i) {
        const auto &wave = (*waves)[i];
        const double time = static_cast<double>(i) / frequency;
        for (size_t ch = 0; ch < numChannels && ch < wave.channelValues.size(); ++ch) {
            out << "chartData[" << ch << "].labels.push(" << time << ");\n";
            out << "chartData[" << ch << "].datasets[0].data.push(" << wave.channelValues[ch] << ");\n";
        }
    }
    
    for (size_t ch = 0; ch < numChannels; ++ch) {
        for (size_t i = 0; i < maxSamples; ++i) {
            const auto &wave = (*waves)[i];
            if (ch < wave.channelValues.size()) {
                const double time = static_cast<double>(i) / frequency;
                if (ch < wave.p_wave_onset.size() && wave.p_wave_onset[ch]) {
                    out << "chartData[" << ch << "].datasets[1].data.push({x: " << time << ", y: " << wave.channelValues[ch] << "});\n";
                }
                if (ch < wave.p_wave_end.size() && wave.p_wave_end[ch]) {
                    out << "chartData[" << ch << "].datasets[2].data.push({x: " << time << ", y: " << wave.channelValues[ch] << "});\n";
                }
                if (ch < wave.qrs_onset.size() && wave.qrs_onset[ch]) {
                    out << "chartData[" << ch << "].datasets[3].data.push({x: " << time << ", y: " << wave.channelValues[ch] << "});\n";
                }
                if (ch < wave.qrs_end.size() && wave.qrs_end[ch]) {
                    out << "chartData[" << ch << "].datasets[4].data.push({x: " << time << ", y: " << wave.channelValues[ch] << "});\n";
                }
                if (ch < wave.t_end.size() && wave.t_end[ch]) {
                    out << "chartData[" << ch << "].datasets[5].data.push({x: " << time << ", y: " << wave.channelValues[ch] << "});\n";
                }
            }
        }
    }
    
    out << "for (let ch = 0; ch < numChannels; ch++) {\n";
    out << "  const ctx = document.getElementById('chart' + ch);\n";
    out << "  new Chart(ctx, {\n";
    out << "    data: chartData[ch],\n";
    out << "    options: {\n";
    out << "      responsive: true,\n";
    out << "      maintainAspectRatio: true,\n";
    out << "      scales: {\n";
    out << "        x: { type: 'linear', title: { display: true, text: 'Time (s)' } },\n";
    out << "        y: { title: { display: true, text: 'Amplitude' } }\n";
    out << "      },\n";
    out << "      plugins: {\n";
    out << "        legend: { display: true },\n";
    out << "        tooltip: { mode: 'nearest', intersect: false }\n";
    out << "      }\n";
    out << "    }\n";
    out << "  });\n";
    out << "}\n";
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}

bool ResultsRepository::ExportHeartClassJSON(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    QJsonObject root;
    root["module"] = "HEART CLASS";
    root["filename"] = filename;
    
    QJsonObject data;
    QJsonObject annotations;
    for (const auto &pair : heart_class_result.annotations) {
        annotations[QString::number(pair.first)] = QString::fromStdString(pair.second);
    }
    data["annotations"] = annotations;
    int nCount = 0, vCount = 0, aCount = 0, otherCount = 0;
    for (const auto &pair : heart_class_result.annotations) {
        if (pair.second == "N") nCount++;
        else if (pair.second == "V") vCount++;
        else if (pair.second == "A") aCount++;
        else otherCount++;
    }
    QJsonObject counts;
    counts["N"] = nCount;
    counts["V"] = vCount;
    counts["A"] = aCount;
    counts["Other"] = otherCount;
    data["counts"] = counts;
    root["data"] = data;

    QJsonDocument doc(root);
    out << doc.toJson();
    file.close();
    return true;
}

bool ResultsRepository::ExportHeartClassCSV(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result, double sampling_frequency) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    out << "Module,HEART CLASS\n";
    out << "Filename," << filename << "\n";
    out << "\n";
    int nCount = 0, vCount = 0, aCount = 0, otherCount = 0;
    for (const auto &pair : heart_class_result.annotations) {
        if (pair.second == "N") nCount++;
        else if (pair.second == "V") vCount++;
        else if (pair.second == "A") aCount++;
        else otherCount++;
    }
    int total = nCount + vCount + aCount + otherCount;
    out << "Class,Count,Percentage\n";
    if (total > 0) {
        out << "N," << nCount << "," << (100.0 * nCount / total) << "\n";
        out << "V," << vCount << "," << (100.0 * vCount / total) << "\n";
        out << "A," << aCount << "," << (100.0 * aCount / total) << "\n";
        out << "Other," << otherCount << "," << (100.0 * otherCount / total) << "\n";
    }
    out << "\n";
    out << "Sample,Time,Class\n";
    for (const auto &pair : heart_class_result.annotations) {
        out << pair.first << "," << (static_cast<double>(pair.first) / sampling_frequency) << "," << QString::fromStdString(pair.second) << "\n";
    }

    file.close();
    return true;
}

bool ResultsRepository::ExportHeartClassHTML(const QString &filepath, const QString &filename, const HeartClassResult &heart_class_result, double sampling_frequency) {
    QFile file(filepath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&file);
    out.setRealNumberPrecision(6);
    
    out << "<!DOCTYPE html>\n";
    out << "<html>\n";
    out << "<head>\n";
    out << "<meta charset=\"UTF-8\">\n";
    out << "<title>EKG Results - HEART CLASS</title>\n";
    out << "<script src=\"https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.min.js\"></script>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }\n";
    out << "h1 { color: #333; border-bottom: 2px solid #4CAF50; padding-bottom: 10px; }\n";
    out << "h2 { color: #555; margin-top: 30px; }\n";
    out << "table { border-collapse: collapse; width: 100%; margin: 20px 0; background-color: white; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
    out << "th, td { border: 1px solid #ddd; padding: 12px; text-align: left; }\n";
    out << "th { background-color: #4CAF50; color: white; font-weight: bold; }\n";
    out << "tr:nth-child(even) { background-color: #f9f9f9; }\n";
    out << "tr:hover { background-color: #f5f5f5; }\n";
    out << ".info { background-color: #e7f3ff; padding: 15px; border-left: 4px solid #2196F3; margin: 20px 0; }\n";
    out << ".chart-container { background-color: white; padding: 20px; margin: 20px 0; box-shadow: 0 2px 4px rgba(0,0,0,0.1); max-width: 600px; }\n";
    out << "</style>\n";
    out << "</head>\n";
    out << "<body>\n";
    out << "<h1>EKG Analysis Results</h1>\n";
    out << "<div class=\"info\">\n";
    out << "<p><strong>Module:</strong> HEART CLASS</p>\n";
    out << "<p><strong>Filename:</strong> " << filename << "</p>\n";
    out << "</div>\n";
    out << "<h2>Heart Class Classification Results</h2>\n";
    int nCount = 0, vCount = 0, aCount = 0, otherCount = 0;
    for (const auto &pair : heart_class_result.annotations) {
        if (pair.second == "N") nCount++;
        else if (pair.second == "V") vCount++;
        else if (pair.second == "A") aCount++;
        else otherCount++;
    }
    int total = nCount + vCount + aCount + otherCount;
    out << "<table>\n";
    out << "<tr><th>Class</th><th>Count</th><th>Percentage</th></tr>\n";
    if (total > 0) {
        out << "<tr><td>N (Normal)</td><td>" << nCount << "</td><td>" << (100.0 * nCount / total) << " %</td></tr>\n";
        out << "<tr><td>V (Ventricular)</td><td>" << vCount << "</td><td>" << (100.0 * vCount / total) << " %</td></tr>\n";
        out << "<tr><td>A (Atrial)</td><td>" << aCount << "</td><td>" << (100.0 * aCount / total) << " %</td></tr>\n";
        out << "<tr><td>Other</td><td>" << otherCount << "</td><td>" << (100.0 * otherCount / total) << " %</td></tr>\n";
    }
    out << "</table>\n";
    
    // Pie Chart
    if (total > 0) {
        out << "<h2>Heart Class Distribution</h2>\n";
        out << "<div class=\"chart-container\">\n";
        out << "<canvas id=\"heartClassChart\"></canvas>\n";
        out << "</div>\n";
    }
    
    out << "<script>\n";
    
    // Pie Chart
    if (total > 0) {
        out << "const heartClassCtx = document.getElementById('heartClassChart').getContext('2d');\n";
        out << "const heartClassData = {\n";
        out << "  labels: ['N (Normal)', 'V (Ventricular)', 'A (Atrial)', 'Other'],\n";
        out << "  datasets: [{\n";
        out << "    data: [" << nCount << ", " << vCount << ", " << aCount << ", " << otherCount << "],\n";
        out << "    backgroundColor: [\n";
        out << "      'rgba(75, 192, 192, 0.6)',\n";
        out << "      'rgba(255, 99, 132, 0.6)',\n";
        out << "      'rgba(255, 206, 86, 0.6)',\n";
        out << "      'rgba(153, 102, 255, 0.6)'\n";
        out << "    ],\n";
        out << "    borderColor: [\n";
        out << "      'rgba(75, 192, 192, 1)',\n";
        out << "      'rgba(255, 99, 132, 1)',\n";
        out << "      'rgba(255, 206, 86, 1)',\n";
        out << "      'rgba(153, 102, 255, 1)'\n";
        out << "    ],\n";
        out << "    borderWidth: 2\n";
        out << "  }]\n";
        out << "};\n";
        out << "new Chart(heartClassCtx, {\n";
        out << "  type: 'pie',\n";
        out << "  data: heartClassData,\n";
        out << "  options: {\n";
        out << "    responsive: true,\n";
        out << "    plugins: {\n";
        out << "      legend: { position: 'right' },\n";
        out << "      tooltip: {\n";
        out << "        callbacks: {\n";
        out << "          label: function(context) {\n";
        out << "            const label = context.label || '';\n";
        out << "            const value = context.parsed || 0;\n";
        out << "            const total = context.dataset.data.reduce((a, b) => a + b, 0);\n";
        out << "            const percentage = ((value / total) * 100).toFixed(1);\n";
        out << "            return label + ': ' + value + ' (' + percentage + '%)';\n";
        out << "          }\n";
        out << "        }\n";
        out << "      }\n";
        out << "    }\n";
        out << "  }\n";
        out << "});\n";
    }
    
    out << "</script>\n";
    out << "</body>\n";
    out << "</html>\n";
    file.close();
    return true;
}
