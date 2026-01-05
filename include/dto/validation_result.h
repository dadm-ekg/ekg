#ifndef EKG_VALIDATION_RESULT_H
#define EKG_VALIDATION_RESULT_H

#include <QString>
#include <vector>

struct ValidationError {
    QString type;
    QString message;
    int lineNumber;
    int columnNumber;
    
    ValidationError(const QString& t, const QString& msg, int line = -1, int col = -1)
        : type(t), message(msg), lineNumber(line), columnNumber(col) {}
};

struct ValidationResult {
    bool isValid;
    std::vector<ValidationError> errors;
    QString summary;
    
    ValidationResult() : isValid(true) {}
    
    void addError(const QString& type, const QString& message, int line = -1, int col = -1) {
        errors.emplace_back(type, message, line, col);
        isValid = false;
    }
    
    QString getFormattedErrors() const {
        if (isValid) return "";
        
        QString result = "Plik niepoprawny. Powody:\n";
        for (const auto& error : errors) {
            result += QString("• %1: %2").arg(error.type, error.message);
            if (error.lineNumber >= 0) {
                result += QString(" (linia %1").arg(error.lineNumber);
                if (error.columnNumber >= 0) {
                    result += QString(", kolumna %1").arg(error.columnNumber);
                }
                result += ")";
            }
            result += "\n";
        }
        return result;
    }
};

#endif

