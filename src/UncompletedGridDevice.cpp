#include "UncompletedGridDevice.h"

namespace CODAR {

    void UncompletedGridDevice::resetAvailableQubits(const bool *available_qubits) {
        memcpy(this->available_qubits, available_qubits, m * n);
        for(int i = 0; i < m; i++) {
            for(int j = 0; j < n; j++) {
                getQubit(i, j).nearbyQubits.clear();
                if(isQubitAvailable(i, j)) {
                    if(j > 0 && isQubitAvailable(i, j - 1))
                        getQubit(i, j).nearbyQubits.emplace_back(i, j - 1);
                    if(j < n - 1 && isQubitAvailable(i, j + 1))
                        getQubit(i, j).nearbyQubits.emplace_back(i, j + 1);
                    if(i > 0 && isQubitAvailable(i - 1, j))
                        getQubit(i, j).nearbyQubits.emplace_back(i - 1, j);
                    if(i < m - 1 && isQubitAvailable(i + 1, j))
                        getQubit(i, j).nearbyQubits.emplace_back(i + 1, j);
                }
            }
        }
    }

    UncompletedGridDevice::~UncompletedGridDevice() {
        delete [] available_qubits;
    }

} // CODAR