#include "SimpleGridDevice.h"

namespace CODAR {
    SimpleGridDevice::~SimpleGridDevice() {
        // Nothing
    }

    bool SimpleGridDevice::isNearBy(int i1, int j1, int i2, int j2) {
        switch(i1 - i2) {
        case -1:
        case 1:
            return j1 == j2;
        case 0:
            return j1 == j2 - 1 || j1 == j2 + 1;
        default:
            return false;
        }
    }

    int SimpleGridDevice::getDistance(int i1, int j1, int i2, int j2) {
        return std::abs(i1 - i2) + std::abs(j1 - j2);
    }

} // CODAR
