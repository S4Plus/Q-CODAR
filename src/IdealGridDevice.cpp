#include "IdealGridDevice.h"

namespace CODAR {
    IdealGridDevice::~IdealGridDevice() {
        // Nothing
    }

    bool IdealGridDevice::isNearBy(int i1, int j1, int i2, int j2) {
        return true;
    }

    int IdealGridDevice::getDistance(int i1, int j1, int i2, int j2) {
        return 1;
    }

} // CODAR
