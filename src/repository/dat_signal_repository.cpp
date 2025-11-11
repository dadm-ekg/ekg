#include "../../include/repository/dat_signal_repository.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <iostream>

std::shared_ptr<SignalDataset> DATSignalRepository::Load(const QString& filename) {
    QFileInfo fileInfo(filename);
    QString baseName = fileInfo.completeBaseName();
    QString dirPath = fileInfo.absolutePath();
    
    QString headerPath = dirPath + "/" + baseName + ".hea";
    QString dataPath = dirPath + "/" + baseName + ".dat";
    
    QFile headerFile(headerPath);
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Error: Cannot open header file: " << headerPath.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }
    
    QTextStream headerStream(&headerFile);
    QString firstLine = headerStream.readLine();
    QStringList parts = firstLine.split(' ', Qt::SkipEmptyParts);
    
    if (parts.size() < 4) {
        std::cerr << "Error: Invalid header format" << std::endl;
        headerFile.close();
        return std::make_shared<SignalDataset>();
    }
    
    int numSignals = parts[1].toInt();
    int frequency = parts[2].toInt();
    int numSamples = parts[3].toInt();
    
    QString signalLine = headerStream.readLine();
    QStringList signalParts = signalLine.split(' ', Qt::SkipEmptyParts);
    
    if (signalParts.size() < 3) {
        std::cerr << "Error: Invalid signal description" << std::endl;
        headerFile.close();
        return std::make_shared<SignalDataset>();
    }
    
    QString gainStr = signalParts[2];
    int gainEndIdx = gainStr.indexOf('(');
    if (gainEndIdx == -1) {
        std::cerr << "Error: Invalid gain format" << std::endl;
        headerFile.close();
        return std::make_shared<SignalDataset>();
    }
    
    double gain = gainStr.left(gainEndIdx).toDouble();
    
    int baselineStartIdx = gainStr.indexOf('(') + 1;
    int baselineEndIdx = gainStr.indexOf(')');
    int baseline = gainStr.mid(baselineStartIdx, baselineEndIdx - baselineStartIdx).toInt();
    
    headerFile.close();
    
    QFile dataFile(dataPath);
    if (!dataFile.open(QIODevice::ReadOnly)) {
        std::cerr << "Error: Cannot open data file: " << dataPath.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }
    
    QByteArray data = dataFile.readAll();
    dataFile.close();
    
    int expectedSize = numSamples * numSignals * 2;
    if (data.size() != expectedSize) {
        std::cerr << "Warning: File size mismatch. Expected: " << expectedSize 
                  << ", Got: " << data.size() << std::endl;
    }
    
    auto dataset = std::make_shared<SignalDataset>();
    dataset->frequency = frequency;
    dataset->values.reserve(numSamples);
    
    const int16_t* rawData = reinterpret_cast<const int16_t*>(data.constData());
    
    for (int i = 0; i < numSamples; ++i) {
        int idx = i * numSignals;
        
        if (idx < data.size() / 2) {
            int16_t adcValue = rawData[idx];
            
            float physicalValue = (static_cast<float>(adcValue) - baseline) / gain;
            
            SignalDatapoint datapoint;
            datapoint.value = physicalValue;
            dataset->values.push_back(datapoint);
        }
    }
    
    std::cout << "Loaded " << dataset->values.size() << " samples at " 
              << dataset->frequency << " Hz from " << filename.toStdString() << std::endl;
    
    return dataset;
}
