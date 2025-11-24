#include <QtCore/QFile>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QString>
#include <QRegularExpression>

#include <iostream>
#include <cstdint>
#include <algorithm>
#include <memory>
#include <vector>
#include <cmath>    
#include <limits>   


struct SignalDataset
{
    int frequency   = 0;  // sampling frequency [Hz]
    int numSignals  = 0;  // number of channels
    int numSamples  = 0;  // number of samples per channel

    std::vector<QString> leadNames;                 // lead names
    std::vector<std::vector<float>> values;        // values[sample][channel]
};

class DATSignalRepository
{
public:
    std::shared_ptr<SignalDataset> Load(const QString& filename);
};


static bool parse_gain_baseline(const QString& line,
                                double& gain,
                                int& baseline,
                                QString& leadNameOut)
{
    const QStringList tokens =
        line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (tokens.isEmpty()) return false;

    // Default outputs
    gain = 1.0;
    baseline = 0;
    leadNameOut = tokens.last();

    for (const QString& t : tokens) {
        int l = t.indexOf('(');
        int r = t.indexOf(')');
        if (l >= 0 && r > l + 1) {
            bool okG=false, okB=false;
            double g = t.left(l).toDouble(&okG);
            int b = t.mid(l+1, r-l-1).toInt(&okB);
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
            bool okG=false;
            double g = tokens[i].left(tokens[i].indexOf('/')).toDouble(&okG);
            if (okG) { gain = (g == 0.0 ? 1.0 : g); gainIdx = i; break; }
        }
    }
    if (gainIdx >= 0) {
        for (int j = gainIdx + 1; j < tokens.size(); ++j) {
            bool okB=false;
            int b = tokens[j].toInt(&okB);
            if (okB) { baseline = b; return true; }
        }
    }

    bool okG=false;
    double g = 0.0;
    for (const QString& t : tokens) {
        if (!okG) {
            g = t.toDouble(&okG);
            if (okG) gain = (g == 0.0 ? 1.0 : g);
        } else {
            bool okB=false;
            int b = t.toInt(&okB);
            if (okB) { baseline = b; return true; }
        }
    }

    return okG;
}


static std::vector<float> resample_linear(const std::vector<float>& src, int targetLen)
{
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

static void interpolate_invalid_inplace(std::vector<float>& v)
{
    const int n = static_cast<int>(v.size());
    if (n == 0) return;

    auto isBad = [](float x){ return !std::isfinite(x); };

    int firstValid = -1;
    for (int i = 0; i < n; ++i) {
        if (!isBad(v[i])) { firstValid = i; break; }
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


std::shared_ptr<SignalDataset> DATSignalRepository::Load(const QString& filename)
{
    QFileInfo fileInfo(filename);
    const QString baseName = fileInfo.completeBaseName();
    const QString dirPath  = fileInfo.absolutePath();

    const QString headerPath = dirPath + "/" + baseName + ".hea";
    const QString dataPath   = dirPath + "/" + baseName + ".dat";

    QFile headerFile(headerPath);
    if (!headerFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "Error: Cannot open header file: "
                  << headerPath.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }

    QTextStream hs(&headerFile);

    const QString firstLine = hs.readLine();
    const QStringList parts =
        firstLine.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

    if (parts.size() < 4) {
        std::cerr << "Error: Invalid header format (first line): "
                  << firstLine.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }

    bool okN=false, okF=false, okS=false;
    const int numSignals = parts[1].toInt(&okN);
    const int frequency  = parts[2].toInt(&okF);
    const int numSamples = parts[3].toInt(&okS);

    if (!okN || !okF || !okS || numSignals <= 0 || frequency <= 0 || numSamples <= 0) {
        std::cerr << "Error: Invalid numbers in header line: "
                  << firstLine.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }

    std::vector<double> gains(numSignals, 1.0);
    std::vector<int> baselines(numSignals, 0);
    std::vector<QString> leadNames(numSignals, "");

    for (int ch = 0; ch < numSignals; /* ++ch inside */) {
        if (hs.atEnd()) {
            std::cerr << "Error: Unexpected end of header while reading channel lines."
                      << std::endl;
            return std::make_shared<SignalDataset>();
        }

        const QString line = hs.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;

        double g=1.0; int b=0; QString lead;
        if (!parse_gain_baseline(line, g, b, lead)) {
            std::cerr << "Error: Cannot parse gain/baseline for channel "
                      << ch << ": " << line.toStdString() << std::endl;
            return std::make_shared<SignalDataset>();
        }

        gains[ch] = (g == 0.0 ? 1.0 : g);
        baselines[ch] = b;
        leadNames[ch] = lead;
        ++ch;
    }

    headerFile.close();

    QFile dataFile(dataPath);
    if (!dataFile.open(QIODevice::ReadOnly)) {
        std::cerr << "Error: Cannot open data file: "
                  << dataPath.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }

    const QByteArray data = dataFile.readAll();
    dataFile.close();

    if (data.isEmpty()) {
        std::cerr << "Error: Data file is empty: "
                  << dataPath.toStdString() << std::endl;
        return std::make_shared<SignalDataset>();
    }

    if (data.size() % static_cast<int>(sizeof(int16_t)) != 0) {
        std::cerr << "Warning: Data size not multiple of 2 bytes. "
                  << "Size=" << data.size() << " bytes." << std::endl;
    }

    const qsizetype totalInt16  =
        data.size() / static_cast<qsizetype>(sizeof(int16_t));
    const qsizetype totalFrames = totalInt16 / numSignals;

    if (totalFrames <= 0) {
        std::cerr << "Error: Not enough data for a single frame. "
                  << "totalInt16=" << totalInt16
                  << ", numSignals=" << numSignals << std::endl;
        return std::make_shared<SignalDataset>();
    }

    if (totalFrames < numSamples) {
        std::cerr << "Warning: Data shorter than header. "
                  << "HeaderSamples=" << numSamples
                  << ", AvailableFrames=" << totalFrames
                  << ". Will interpolate to header length."
                  << std::endl;
    }

    const int framesAvailable =
        static_cast<int>(std::min<qsizetype>(totalFrames, numSamples));
    const int16_t* raw =
        reinterpret_cast<const int16_t*>(data.constData());

    std::vector<std::vector<float>> temp(framesAvailable,
                                         std::vector<float>(numSignals, 0.0f));

    int nonFiniteCount = 0;
    const float NaN = std::numeric_limits<float>::quiet_NaN();

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
                physical = NaN;  
            }

            temp[i][ch] = physical;
        }
    }

    if (nonFiniteCount > 0) {
        for (int ch = 0; ch < numSignals; ++ch) {
            std::vector<float> col(framesAvailable);
            for (int i = 0; i < framesAvailable; ++i)
                col[i] = temp[i][ch];

            interpolate_invalid_inplace(col);

            for (int i = 0; i < framesAvailable; ++i)
                temp[i][ch] = col[i];
        }

        std::cerr << "Warning: detected " << nonFiniteCount
                  << " non-finite samples (NaN/Inf); interpolated in time."
                  << std::endl;
    }

    auto dataset = std::make_shared<SignalDataset>();
    dataset->frequency  = frequency;
    dataset->numSignals = numSignals;
    dataset->numSamples = numSamples;         
    dataset->leadNames  = leadNames;
    dataset->values.assign(numSamples,
                           std::vector<float>(numSignals, 0.0f));

    if (framesAvailable == numSamples) {
        dataset->values = temp;
    } else {
        for (int ch = 0; ch < numSignals; ++ch) {
            std::vector<float> src(framesAvailable);
            for (int i = 0; i < framesAvailable; ++i)
                src[i] = temp[i][ch];

            std::vector<float> interp =
                resample_linear(src, numSamples);

            for (int i = 0; i < numSamples; ++i)
                dataset->values[i][ch] = interp[i];
        }
    }

    std::cout << "Loaded " << dataset->numSamples << " samples x "
              << dataset->numSignals << " channels at "
              << dataset->frequency << " Hz from "
              << filename.toStdString() << std::endl;

    return dataset;
}
