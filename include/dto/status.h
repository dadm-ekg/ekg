#ifndef EKG_STATUS_H
#define EKG_STATUS_H

enum Status {
    // Domyślny stan programu, oznacza, że żaden plik nie jest załadowany
    idle,
    // Stan w którym dane są ładowane
    loading,
    // Stan, w którym dane w GetData() są dostępne (ale nadal GetFilteredData() może zwracać NULLPTR)
    loaded,
    // Tymczasowy stan, który informuje, że wykonywane jest jakieś obliczenie (filtrowanie, wykrywanie itp.)
    processing
};

#endif //EKG_STATUS_H
