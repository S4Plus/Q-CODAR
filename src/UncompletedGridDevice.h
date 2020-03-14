#ifndef UNCOMPLETED_GRID_DEVICE_H
#define UNCOMPLETED_GRID_DEVICE_H

/**
 * UncompletedGridDevice is a derived class of SimpleGridDevice
 * Compared with SimpleGridDevice, some qubits are NOT AVAILABLE, but the adjacent relation is the same
 * e.g. google Sycamore q54
 */

#include <cstring>
#include "SimpleGridDevice.h"

namespace CODAR
{
    class UncompletedGridDevice : public SimpleGridDevice {
    public:
        inline UncompletedGridDevice(int m, int n, const bool *available_qubits)
            : SimpleGridDevice(m, n, false) {
            is_simple_grid_device = true;
            this->available_qubits = new bool[m * n];
            resetAvailableQubits(available_qubits);
        }

        void resetAvailableQubits(const bool *available_qubits);

        ~UncompletedGridDevice() override;

        inline bool isQubitAvailable(int i, int j) {
            return available_qubits[i * n + j];
        }

    private:
        bool *available_qubits;
    };



} // namespace CODAR



#endif