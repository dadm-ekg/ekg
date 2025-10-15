#ifndef EKG_APPLICATION_SERVICE_H
#define EKG_APPLICATION_SERVICE_H

#include "../dto/signal_range.h"
#include "../dto/status.h"
#include "../dto/filter_method.h"
#include "../model/signal_datapoint.h"

class IApplicationService {
public:
    virtual ~IApplicationService() = default;

    // Ta metoda służy do podstawowego załadowania pliku za pomocą CSV
    virtual bool Load(QString filename) = 0;

    // Ta metoda zwraca długość wektora wartości
    virtual int GetLength() = 0;

    // Ta metoda zwraca częstotliwość zbioru danych w hertzach
    virtual int GetFrequency() = 0;

    // W aplikacji istnieje koncept ViewRange - jest to zakres aktualnie wyświetlanego wykresu sygnału
    // SignalRange.start wskazuje od którego indeksu wektora powinny być wyświetlane dane
    // SignalRange.end wskazuje końcowy indeks
    virtual SignalRange GetViewRange() = 0;

    // Użytkownik powinien mieć możliwość ustawienia zakresu w trakcie korzystania z programu
    virtual void SetViewRange(SignalRange range) = 0;

    // Zwraca sygnał w określonym zakresie przez SignalRange
    // Ta metoda powinna być użyta do wyrysowania wykresu sygnału
    // Ten sygnał nie jest przefiltrowany. W przypadku gdy Status=idle, ta metoda zwraca NULLPTR
    virtual std::shared_ptr<std::vector<SignalDatapoint>> GetData() = 0;
    // Metoda działająca analogicznie do GetData(), przy czym zwraca sygnał przefiltrowany.
    // Jeżeli użytkownik nie uruchomił żadnego algorytmu filtracji, metoda zwraca NULLPTR
    virtual std::shared_ptr<std::vector<SignalDatapoint>> GetFilteredData() = 0;

    // Zwraca jeden ze statusów
    virtual Status GetStatus() = 0;

    // Uruchamia filtrowanie wedle zadanego algorytmu. Zwraca prawdę jeżeli filtrowanie się udało.
    // Po udanym wykonaniu tej metody, GetFilteredData() zwraca sensowne dane.
    virtual bool RunFiltering(FilterMethod method) = 0;
};

#endif //EKG_APPLICATION_SERVICE_H
