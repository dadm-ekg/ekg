#include "../../include/repository/dat_signal_repository.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QRegularExpression>

#include <iostream>
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>
#include <limits>

#include "../../include/model/signal_dataset.h"
#include "../../include/model/signal_datapoint.h"
#include "../../include/dto/validation_result.h"


static bool parse_gain_baseline(const QString &line,
                                double &gain,
                                int &baseline,
                                QString &leadNameOut,
                                int &formatOut) {
    const QStringList tokens =
            line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.isEmpty()) return false;

    gain = 1.0;
    baseline = 0;
    formatOut = 16;
    leadNameOut = tokens.last();

    if (tokens.size() >= 5) {
        bool okFmt = false;
        int fmt = tokens[1].toInt(&okFmt);
        
        bool isPlainGain = false;
        double plainGainValue = 0.0;
        if (tokens.size() > 2 && !tokens[2].contains('(') && !tokens[2].contains('/')) {
            bool okG = false;
            plainGainValue = tokens[2].toDouble(&okG);
            isPlainGain = okG;
        }
        
        if (okFmt && isPlainGain && (fmt == 212 || fmt == 16 || fmt == 80 || fmt == 310 || fmt == 311)) {
            formatOut = fmt;
            gain = (plainGainValue == 0.0 ? 1.0 : plainGainValue);
            
            if (tokens.size() >= 5) {
                bool okBaseline = false;
                int b = tokens[4].toInt(&okBaseline);
                if (okBaseline) {
                    baseline = b;
                }
            }
            
            return true;
        }
    }

    for (const QString &t: tokens) {
        int l = t.indexOf('(');
        int r = t.indexOf(')');
        if (l >= 0 && r > l + 1) {
            bool okG = false, okB = false;
            double g = t.left(l).toDouble(&okG);
            int b = t.mid(l + 1, r - l - 1).toInt(&okB);
            if (okG && okB) {
                gain = (g == 0.0 ? 1.0 : g);
                baseline = b;
                return true;
            }
        }
    }

    int gainIdx = -1;
    for (int i = 0; i < tokens.size(); ++i) {
        if (tokens[i].contains('/')) {
            bool okG = false;
            double g = tokens[i].left(tokens[i].indexOf('/')).toDouble(&okG);
            if (okG) {
                gain = (g == 0.0 ? 1.0 : g);
                gainIdx = i;
                break;
            }
        }
    }
    if (gainIdx >= 0) {
        for (int j = gainIdx + 1; j < tokens.size(); ++j) {
            bool okB = false;
            int b = tokens[j].toInt(&okB);
            if (okB) {
                baseline = b;
                return true;
            }
        }
    }

    bool okG = false;
    double g = 0.0;
    for (const QString &t: tokens) {
        if (!okG) {
            g = t.toDouble(&okG);
            if (okG) gain = (g == 0.0 ? 1.0 : g);
        } else {
            bool okB = false;
            int b = t.toInt(&okB);
            if (okB) {
                baseline = b;
                return true;
            }
        }
    }

    return okG;
}


static std::vector<float> resample_linear(const std::vector<float> &src, int targetLen) {
    const int srcLen = static_cast<int>(src.size());
    std::vector<float> out(targetLen, 0.0f);

    if (targetLen <= 0) return out;
    if (srcLen == 0) return out;

    if (srcLen == targetLen) {
        return src;
    }
    if (srcLen == 1) {
        std::fill(out.begin(), out.end(), src[0]);
        return out;
    }

    const double scale =
            static_cast<double>(srcLen - 1) / static_cast<double>(targetLen - 1);

    for (int i = 0; i < targetLen; ++i) {
        double pos = i * scale;
        int left = static_cast<int>(std::floor(pos));
        int right = std::min(left + 1, srcLen - 1);
        double t = pos - left;
        out[i] = static_cast<float>((1.0 - t) * src[left] + t * src[right]);
    }

    return out;
}

static void interpolate_invalid_inplace(std::vector<float> &v) {
    const int n = static_cast<int>(v.size());
    if (n == 0) return;

    auto isBad = [](float x) { return !std::isfinite(x); };

    int firstValid = -1;
    for (int i = 0; i < n; ++i) {
        if (!isBad(v[i])) {
            firstValid = i;
            break;
        }
    }
    if (firstValid == -1) {
        std::fill(v.begin(), v.end(), 0.0f);
        return;
    }


    for (int i = 0; i < firstValid; ++i) {
        v[i] = v[firstValid];
    }

    int lastValid = firstValid;
    int i = firstValid + 1;

    while (i < n) {
        if (!isBad(v[i])) {
            lastValid = i;
            ++i;
            continue;
        }

        int startBad = i;
        int endBad = i;
        while (endBad < n && isBad(v[endBad])) {
            ++endBad;
        }

        if (endBad == n) {
            for (int k = startBad; k < n; ++k) {
                v[k] = v[lastValid];
            }
            break;
        }

        float leftVal = v[lastValid];
        float rightVal = v[endBad];
        int gap = endBad - lastValid;

        for (int k = 1; k < gap; ++k) {
            float t = static_cast<float>(k) / static_cast<float>(gap);
            v[lastValid + k] = (1.0f - t) * leftVal + t * rightVal;
        }

        lastValid = endBad;
        i = endBad + 1;
    }
}


static bool isValidNumber(const QString& str, bool allowFloat = false) {
    if (str.isEmpty()) return false;
    
    bool hasDigits = false;
    bool hasDot = false;
    bool hasSign = false;
    
    for (int i = 0; i < str.length(); ++i) {
        QChar c = str[i];
        if (c.isDigit()) {
            hasDigits = true;
        } else if (c == '.' || c == ',') {
            if (!allowFloat || hasDot) return false;
            hasDot = true;
        } else if (c == '-' || c == '+') {
            if (hasSign || i > 0) return false;
            hasSign = true;
        } else if (c == 'e' || c == 'E') {
            if (!allowFloat || i == 0 || i == str.length() - 1) return false;
        } else {
            return false;
        }
    }
    
    return hasDigits;
}

static std::vector<std::vector<int16_t>> decodeFormat212(const QByteArray &data, int numSignals, int numSamples) {
    std::vector<std::vector<int16_t>> samples;
    
    if (numSignals != 2) {
        return samples;
    }
    
    const uint8_t* raw = reinterpret_cast<const uint8_t*>(data.constData());
    const qsizetype dataSize = data.size();
    qsizetype byteIdx = 0;
    
    samples.reserve(numSamples);
    
    while (byteIdx + 2 < dataSize && static_cast<int>(samples.size()) < numSamples) {
        uint8_t b0 = raw[byteIdx];
        uint8_t b1 = raw[byteIdx + 1];
        uint8_t b2 = raw[byteIdx + 2];
        
        int16_t s0 = (b0 & 0xFF) | ((b1 & 0x0F) << 8);
        if (s0 & 0x800) s0 |= 0xF000;
        
        int16_t s1 = ((b1 & 0xF0) >> 4) | ((b2 & 0xFF) << 4);
        if (s1 & 0x800) s1 |= 0xF000;
        
        std::vector<int16_t> frame = {s0, s1};
        samples.push_back(frame);
        
        byteIdx += 3;
    }
    
    return samples;
}

std::shared_ptr<SignalDataset> DATSignalRepository::Load(const QString &filename) {
    last_validation_result_ = ValidationResult();
    
    QFileInfo fileInfo(filename);
    const QString baseName = fileInfo.completeBaseName();
    const QString dirPath = fileInfo.absolutePath();

    const QString headerPath = dirPath + "/" + baseName + ".hea";
    const QString dataPath = dirPath + "/" + baseName + ".dat";

    QFile headerFile(headerPath);
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        last_validation_result_.addError("Błąd otwarcia pliku", 
            QString("Nie można otworzyć pliku nagłówka: %1").arg(headerPath));
        return nullptr;
    }

    QTextStream hs(&headerFile);
    int lineNumber = 0;

    const QString firstLine = hs.readLine();
    lineNumber++;
    const QStringList parts =
            firstLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (parts.size() < 4) {
        last_validation_result_.addError("Nieprawidłowy format nagłówka", 
            QString("Pierwsza linia powinna zawierać co najmniej 4 elementy, znaleziono: %1").arg(parts.size()), 
            lineNumber);
        return nullptr;
    }

    bool okN = false, okF = false, okS = false;
    
    if (!isValidNumber(parts[1], false)) {
        last_validation_result_.addError("Nieprawidłowa wartość liczbowa", 
            QString("Liczba sygnałów zawiera nieprawidłowe znaki: '%1'").arg(parts[1]), 
            lineNumber, 1);
    }
    if (!isValidNumber(parts[2], false)) {
        last_validation_result_.addError("Nieprawidłowa wartość liczbowa", 
            QString("Częstotliwość próbkowania zawiera nieprawidłowe znaki: '%1'").arg(parts[2]), 
            lineNumber, 2);
    }
    if (!isValidNumber(parts[3], false)) {
        last_validation_result_.addError("Nieprawidłowa wartość liczbowa", 
            QString("Liczba próbek zawiera nieprawidłowe znaki: '%1'").arg(parts[3]), 
            lineNumber, 3);
    }
    
    const int numSignals = parts[1].toInt(&okN);
    const int frequency = parts[2].toInt(&okF);
    const int numSamples = parts[3].toInt(&okS);

    if (!okN || !okF || !okS || numSignals <= 0 || frequency <= 0 || numSamples <= 0) {
        if (last_validation_result_.isValid) {
            last_validation_result_.addError("Nieprawidłowe wartości liczbowe", 
                QString("Nie można przekonwertować wartości na liczby całkowite lub wartości są nieprawidłowe (numSignals=%1, frequency=%2, numSamples=%3)")
                    .arg(numSignals).arg(frequency).arg(numSamples), 
                lineNumber);
        }
        return nullptr;
    }

    std::vector<double> gains(numSignals, 1.0);
    std::vector<int> baselines(numSignals, 0);
    std::vector<QString> leadNames(numSignals, "");
    std::vector<int> formats(numSignals, 16);

    for (int ch = 0; ch < numSignals; /* ++ch inside */) {
        if (hs.atEnd()) {
            last_validation_result_.addError("Nieprawidłowy nagłówek", 
                QString("Nieoczekiwany koniec pliku podczas odczytu linii kanału %1").arg(ch), 
                lineNumber);
            return nullptr;
        }

        const QString line = hs.readLine().trimmed();
        lineNumber++;
        if (line.isEmpty() || line.startsWith('#')) continue;

        double g = 1.0;
        int b = 0;
        QString lead;
        int fmt = 16;
        if (!parse_gain_baseline(line, g, b, lead, fmt)) {
            last_validation_result_.addError("Nieprawidłowy format linii kanału", 
                QString("Nie można sparsować wartości gain/baseline dla kanału %1: '%2'").arg(ch).arg(line), 
                lineNumber);
            return nullptr;
        }
        
        if (!std::isfinite(g)) {
            last_validation_result_.addError("Nieprawidłowa wartość (NaN/Inf)", 
                QString("Wartość gain dla kanału %1 jest nieprawidłowa (NaN lub Inf)").arg(ch), 
                lineNumber);
        }
        if (!std::isfinite(static_cast<double>(b))) {
            last_validation_result_.addError("Nieprawidłowa wartość (NaN/Inf)", 
                QString("Wartość baseline dla kanału %1 jest nieprawidłowa (NaN lub Inf)").arg(ch), 
                lineNumber);
        }

        gains[ch] = (g == 0.0 ? 1.0 : g);
        baselines[ch] = b;
        leadNames[ch] = lead;
        formats[ch] = fmt;
        ++ch;
    }

    headerFile.close();
    
    int dataFormat = formats.empty() ? 16 : formats[0];

    QFile dataFile(dataPath);
    if (!dataFile.open(QIODevice::ReadOnly)) {
        last_validation_result_.addError("Błąd otwarcia pliku", 
            QString("Nie można otworzyć pliku danych: %1").arg(dataPath));
        return nullptr;
    }

    const QByteArray data = dataFile.readAll();
    dataFile.close();

    if (data.isEmpty()) {
        last_validation_result_.addError("Pusty plik", 
            QString("Plik danych jest pusty: %1").arg(dataPath));
        return nullptr;
    }

    std::vector<std::vector<float>> temp;
    int framesAvailable = 0;
    int nonFiniteCount = 0;
    int nanCount = 0;
    int infCount = 0;
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    
    if (dataFormat == 212 && numSignals == 2) {
        auto decoded = decodeFormat212(data, numSignals, numSamples);
        framesAvailable = static_cast<int>(decoded.size());
        
        if (framesAvailable <= 0) {
            last_validation_result_.addError("Niewystarczające dane", 
                QString("Za mało danych dla formatu 212"));
            return nullptr;
        }
        
        temp.resize(framesAvailable, std::vector<float>(numSignals, 0.0f));
        
        for (int i = 0; i < framesAvailable; ++i) {
            for (int ch = 0; ch < numSignals; ++ch) {
                const int16_t adc = decoded[i][ch];
                float physical = static_cast<float>(
                    (static_cast<double>(adc) - baselines[ch]) / gains[ch]
                );
                
                if (!std::isfinite(physical)) {
                    ++nonFiniteCount;
                    if (std::isnan(physical)) {
                        ++nanCount;
                    } else if (std::isinf(physical)) {
                        ++infCount;
                    }
                    physical = NaN;
                }
                
                temp[i][ch] = physical;
            }
        }
    } else {
        if (data.size() % static_cast<int>(sizeof(int16_t)) != 0) {
            std::cerr << "Warning: Data size not multiple of 2 bytes. "
                    << "Size=" << data.size() << " bytes." << std::endl;
        }

        const qsizetype totalInt16 =
                data.size() / static_cast<qsizetype>(sizeof(int16_t));
        const qsizetype totalFrames = totalInt16 / numSignals;

        if (totalFrames <= 0) {
            last_validation_result_.addError("Niewystarczające dane", 
                QString("Za mało danych dla pojedynczej ramki. totalInt16=%1, numSignals=%2")
                    .arg(totalInt16).arg(numSignals));
            return nullptr;
        }

        if (totalFrames < numSamples) {
            std::cerr << "Warning: Data shorter than header. "
                    << "HeaderSamples=" << numSamples
                    << ", AvailableFrames=" << totalFrames
                    << ". Will interpolate to header length."
                    << std::endl;
        }

        framesAvailable =
                static_cast<int>(std::min<qsizetype>(totalFrames, numSamples));
        const int16_t *raw =
                reinterpret_cast<const int16_t *>(data.constData());

        temp.resize(framesAvailable, std::vector<float>(numSignals, 0.0f));

        for (int i = 0; i < framesAvailable; ++i) {
            const qsizetype base = static_cast<qsizetype>(i) * numSignals;
            for (int ch = 0; ch < numSignals; ++ch) {
                const qsizetype idx = base + ch;
                if (idx >= totalInt16) break;

                const int16_t adc = raw[idx];
                float physical = static_cast<float>(
                    (static_cast<double>(adc) - baselines[ch]) / gains[ch]
                );

                if (!std::isfinite(physical)) {
                    ++nonFiniteCount;
                    if (std::isnan(physical)) {
                        ++nanCount;
                    } else if (std::isinf(physical)) {
                        ++infCount;
                    }
                    physical = NaN;
                }

                temp[i][ch] = physical;
            }
        }
    }

    if (nonFiniteCount > 0) {
        QString errorMsg = QString("Wykryto %1 nieprawidłowych wartości").arg(nonFiniteCount);
        if (nanCount > 0) {
            errorMsg += QString(" (%1 NaN").arg(nanCount);
            if (infCount > 0) {
                errorMsg += QString(", %1 Inf").arg(infCount);
            }
            errorMsg += ")";
        } else if (infCount > 0) {
            errorMsg += QString(" (%1 Inf)").arg(infCount);
        }
        errorMsg += ". Wartości zostały interpolowane.";
        
        last_validation_result_.addError("Nieprawidłowe wartości (NaN/Inf)", errorMsg);
        
        for (int ch = 0; ch < numSignals; ++ch) {
            std::vector<float> col(framesAvailable);
            for (int i = 0; i < framesAvailable; ++i)
                col[i] = temp[i][ch];

            interpolate_invalid_inplace(col);

            for (int i = 0; i < framesAvailable; ++i)
                temp[i][ch] = col[i];
        }
    }

    auto dataset = std::make_shared<SignalDataset>();
    dataset->frequency = frequency;
    dataset->values.resize(numSamples);

    if (framesAvailable == numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            dataset->values[i].channelValues = temp[i];
        }
    } else {
        for (int ch = 0; ch < numSignals; ++ch) {
            std::vector<float> src(framesAvailable);
            for (int i = 0; i < framesAvailable; ++i)
                src[i] = temp[i][ch];

            std::vector<float> interp =
                    resample_linear(src, numSamples);

            for (int i = 0; i < numSamples; ++i) {
                if (dataset->values[i].channelValues.empty()) {
                    dataset->values[i].channelValues.resize(numSignals, 0.0f);
                }
                dataset->values[i].channelValues[ch] = interp[i];
            }
        }
    }

    std::cout << "Loaded " << dataset->values.size() << " samples x "
            << numSignals << " channels at "
            << dataset->frequency << " Hz from "
            << filename.toStdString() << std::endl;

    return dataset;
}
