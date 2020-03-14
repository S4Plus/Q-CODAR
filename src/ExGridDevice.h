// ExGridDevice.h
/**
 * ExGirdDevice is an extension of the grid device
 * The qubit layout is the same to BasicGridDevice
 * But the adjacency between qubits can be different 
 * for example, qubit at (i, j) may be not adjacent to (i, j + 1)
 * but(i1 ,j1) and (i2, j2) may be adjacent
 * e.g. ibm tokyo q20 and ibm Rochester q53
 * ExGirdDevice is a derived class of BasicGridDevice
 */
#ifndef EX_GRID_DEVICE_H
#define EX_GRID_DEVICE_H

#include <vector>
#include "BasicGridDevice.h"

namespace CODAR {

    class ExGridDevice : public BasicGridDevice {
    public:
        /**
         * qpairs: set of adjacent qubit pairs
         *      whose elements are like <q1, q2>，q1, q2 ∈ [0, m * n)
         *      q1 is at (q1 / n, q1 % n)
         *      so is q2
         */
        ExGridDevice(int m, int n, std::vector<std::pair<int, int>> &qpairs);

        ~ExGridDevice() override;

        bool isNearBy(int i1, int j1, int i2, int j2) override;

        int getDistance(int i1, int j1, int i2, int j2) override;

    protected:
        int *dist_mat;
        int qcount;
    };


} // CODAR

#endif