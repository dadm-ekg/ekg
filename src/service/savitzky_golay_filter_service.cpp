#include "../../include/service/savitzky_golay_filter_service.h"
#include "../../include/model/signal_datapoint.h"
#include <cmath>
#include <iostream>
#include <vector>

std::vector<double> SavitzkyGolayFilterService::computeCoefficients(int windowSize, int polynomialOrder) {
    int half = windowSize / 2;
    int n = windowSize;
    
    std::vector<std::vector<double>> A(n, std::vector<double>(polynomialOrder + 1));
    for (int i = 0; i < n; ++i) {
        double x = i - half;
        for (int j = 0; j <= polynomialOrder; ++j) {
            A[i][j] = std::pow(x, j);
        }
    }
    
    std::vector<std::vector<double>> AtA(polynomialOrder + 1, std::vector<double>(polynomialOrder + 1, 0.0));
    for (int i = 0; i <= polynomialOrder; ++i) {
        for (int j = 0; j <= polynomialOrder; ++j) {
            for (int k = 0; k < n; ++k) {
                AtA[i][j] += A[k][i] * A[k][j];
            }
        }
    }
    
    std::vector<std::vector<double>> inv(polynomialOrder + 1, std::vector<double>(polynomialOrder + 1, 0.0));
    for (int i = 0; i <= polynomialOrder; ++i) {
        inv[i][i] = 1.0;
    }
    
    for (int col = 0; col <= polynomialOrder; ++col) {
        double pivot = AtA[col][col];
        if (std::abs(pivot) < 1e-10) {
            for (int row = col + 1; row <= polynomialOrder; ++row) {
                if (std::abs(AtA[row][col]) > std::abs(pivot)) {
                    std::swap(AtA[col], AtA[row]);
                    std::swap(inv[col], inv[row]);
                    pivot = AtA[col][col];
                    break;
                }
            }
        }
        
        for (int j = 0; j <= polynomialOrder; ++j) {
            AtA[col][j] /= pivot;
            inv[col][j] /= pivot;
        }
        
        for (int row = 0; row <= polynomialOrder; ++row) {
            if (row != col) {
                double factor = AtA[row][col];
                for (int j = 0; j <= polynomialOrder; ++j) {
                    AtA[row][j] -= factor * AtA[col][j];
                    inv[row][j] -= factor * inv[col][j];
                }
            }
        }
    }
    
    std::vector<double> coeffs(n, 0.0);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j <= polynomialOrder; ++j) {
            coeffs[i] += inv[0][j] * A[i][j];
        }
    }
    
    return coeffs;
}

std::vector<SignalDatapoint> SavitzkyGolayFilterService::Filter(const std::vector<SignalDatapoint>& values, int windowSize, int polynomialOrder) {
    std::vector<SignalDatapoint> filtered(values.size());
    
    if (windowSize < 3) windowSize = 3;
    if (windowSize % 2 == 0) windowSize++;
    if (polynomialOrder < 0) polynomialOrder = 0;
    if (polynomialOrder >= windowSize) polynomialOrder = windowSize - 1;
    
    if (values.size() < static_cast<size_t>(windowSize))
        return values;
    
    if (values.empty() || values[0].channelValues.empty())
        return values;
    
    const size_t numChannels = values[0].channelValues.size();
    
    std::vector<double> h = computeCoefficients(windowSize, polynomialOrder);
    int half = windowSize / 2;
    
    for (size_t i = 0; i < values.size(); ++i) {
        filtered[i].channelValues.resize(numChannels);
        
        for (size_t ch = 0; ch < numChannels; ++ch) {
            double acc = 0.0;
            
            for (int k = -half; k <= half; ++k) {
                int idx = static_cast<int>(i) + k;
                
                if (idx < 0) idx = 0;
                if (idx >= static_cast<int>(values.size())) idx = static_cast<int>(values.size()) - 1;
                
                acc += h[k + half] * values[idx].channelValues[ch];
            }
            
            filtered[i].channelValues[ch] = static_cast<float>(acc);
        }
    }
    
    std::cout << "Savitzky-Golay filter finished (window=" << windowSize << ", order=" << polynomialOrder << ")" << std::endl;
    
    return filtered;
}

