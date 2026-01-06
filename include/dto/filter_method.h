#ifndef EKG_FILTER_METHOD_H
#define EKG_FILTER_METHOD_H

enum FilterMethod {
    MovingAverage = 0,
    Butterworth = 1,
    SavitzkyGolay = 2
};
#endif //EKG_FILTER_METHOD_H
