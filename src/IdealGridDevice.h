#ifndef IDEAL_GRID_DEVICE_H
#define IDEAL_GRID_DEVICE_H

/**
 * IdealGridDevice is a derived class of BasicGridDevice
 * Any pair of qubits are adjacent
 * e.g. ion-trap quantum devices
 */

#include "BasicGridDevice.h"

namespace CODAR
{

    class IdealGridDevice : public BasicGridDevice {
    public:
        inline IdealGridDevice(int m, int n)
            : BasicGridDevice(m, n) {
        }

        ~IdealGridDevice() override;

        bool isNearBy(int i1, int j1, int i2, int j2) override;

        int getDistance(int i1, int j1, int i2, int j2) override;
    };



} // namespace CODAR



#endif