#ifndef SIMPLE_GRID_DEVICE_H
#define SIMPLE_GRID_DEVICE_H

/**
 * SimpleGridDevice
 * A derived class of BasicGridDevice
 * The two qubits are adjacent when their Manhattan distance is 1.
 */

#include "BasicGridDevice.h"

namespace CODAR
{

    class SimpleGridDevice : public BasicGridDevice {
    public:
        inline SimpleGridDevice(int m, int n)
            : BasicGridDevice(m, n) {
            is_simple_grid_device = true;
            for(int i = 0; i < m; i++) {
                for(int j = 0; j < n; j++) {
                    if(j > 0)
                        getQubit(i, j).nearbyQubits.emplace_back(i, j - 1);
                    if(j < n - 1)
                        getQubit(i, j).nearbyQubits.emplace_back(i, j + 1);
                    if(i > 0)
                        getQubit(i, j).nearbyQubits.emplace_back(i - 1, j);
                    if(i < m - 1)
                        getQubit(i, j).nearbyQubits.emplace_back(i + 1, j);
                }
            }
        }

        ~SimpleGridDevice() override;

        bool isNearBy(int i1, int j1, int i2, int j2) override;

        int getDistance(int i1, int j1, int i2, int j2) override;

    protected:
        inline SimpleGridDevice(int m, int n, bool init)
            : BasicGridDevice(m, n) {
            is_simple_grid_device = true;
            if(init) {
                for(int i = 0; i < m; i++) {
                    for(int j = 0; j < n; j++) {
                        if(j > 0)
                            getQubit(i, j).nearbyQubits.emplace_back(i, j - 1);
                        if(j < n - 1)
                            getQubit(i, j).nearbyQubits.emplace_back(i, j + 1);
                        if(i > 0)
                            getQubit(i, j).nearbyQubits.emplace_back(i - 1, j);
                        if(i < m - 1)
                            getQubit(i, j).nearbyQubits.emplace_back(i + 1, j);
                    }
                }
            }
        }
    };



} // namespace CODAR



#endif