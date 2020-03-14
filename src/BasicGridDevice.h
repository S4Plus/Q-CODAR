#ifndef BASIC_GRID_DEVICE_H
#define BASIC_GRID_DEVICE_H

/**
 * A BasicGirdDevice is a device model that qubits put on nodes of a rectangular grid
 * It is a virtual class because we haven't dicided how qubits connecting with each other
 * For the simpliest situation, see SimpleGridDevice
 */

#include <iostream>
#include <assert.h>
#include <vector>
#include "basis.h"


// gate duration, using MACROs for quick prototype
#ifndef SWAP_TIME
#define SWAP_TIME   6
#endif
#ifndef CNOT_TIME
#define CNOT_TIME   2
#endif
#ifndef RX_TIME
#define RX_TIME     1
#endif

// default size of a device
#ifndef DEVICE_M
#define DEVICE_M 2
#define DEVICE_N 8
#endif
// 6 * 6
// 2 * 8
// 4 * 5


namespace CODAR
{
    
class BasicGridDevice {
    public:
        struct PhysicalQubit {
            int map_to = -1;    // which logical qubit maps to it, -1 if no
            int time;           // time when it become free again, the qubit lock
            std::vector<std::pair<int, int>> nearbyQubits;
        };



        /**
         * m, n: the grid side length
         */     
        BasicGridDevice(int m, int n);

        /**
         * clear all properties except gird side length
         * re-initialize
         */
        void clear();

        virtual ~BasicGridDevice();

        // -------------------------------- get properties --------------------------------
        inline int getM() {
            return m;
        }

        inline int getN() {
            return n;
        }

        // -------------------------------- get objects --------------------------------
        /**
         * get the qubit on (i, j)
         */
        inline auto &getQubit(int i, int j) {
            assert(i >= 0 && i < m);
            assert(j >= 0 && j < n);
            return qubits[i * n + j];
        }

        // -------------------------------- apply gates --------------------------------
        /**
         * Determine whether a double-qubit gate can be applied
         */
        inline bool canApplyGate(int i1, int j1, int i2, int j2, int /* time */) {
            return getQubit(i1, j1).time <= current_time
                && getQubit(i2, j2).time <= current_time;
        }

        /**
         * Determine whether a single-qubit gate can be applied
         */
        inline bool canApplyGate(int i, int j, int /* time */) {
            return getQubit(i ,j).time <= current_time;
        }

        inline bool canApplyRx(int i, int j) {
            return canApplyGate(i, j, RX_TIME);
        }

        inline bool canSwap(int i1, int j1, int i2, int j2) {
            return canApplyGate(i1, j1, i2, j2, SWAP_TIME) && isNearBy(i1, j1, i2, j2);
        }


        inline bool canCNot(int i1, int j1, int i2, int j2) {
            return canApplyGate(i1, j1, i2, j2, CNOT_TIME);
        }

        // apply* : apply gate and comsume time

        inline void applyRx(int i, int j) {
            auto &t = getQubit(i, j).time;
            t += RX_TIME;
            qubit_max_time = std::max(qubit_max_time, t);
        }

        /**
         * apply a double-qubit gate
         */
        inline void applyGate(int i1, int j1, int i2, int j2, int time) {
            DEBUG_LOG("applying two-qubit gate");
            DEBUG_DO(std::cerr << "DEBUG: (" << i1 << ", " << j1 << ") with (" << i2 << ", " << j2 << "),"
                " gate time = " << time << std::endl);
            DEBUG_PRINT(current_time);
            auto &t1 = getQubit(i1, j1).time;
            auto &t2 = getQubit(i2, j2).time;
            DEBUG_DO(std::cerr << "DEBUG: before applying, t1 = " << t1 << ", t2 = " << t2 << std::endl);
            assert(t1 <= current_time && t2 <= current_time);
            t1 = t2 = std::max(t1, t2) + time;
            qubit_max_time = std::max(qubit_max_time, t1);
        }

        inline void applyCNot(int i1, int j1, int i2, int j2) {
            applyGate(i1, j1, i2, j2, CNOT_TIME);
        }

        inline void applySwap(int i1, int j1, int i2, int j2) {
            assert(isNearBy(i1, j1, i2, j2));
            applyGate(i1, j1, i2, j2, SWAP_TIME);
            std::swap(getQubit(i1, j1).map_to, getQubit(i2, j2).map_to);
        }

        // Check if two qubits are adjacent, location of qubits are (i1, j1) and (i2, j2)
        virtual bool isNearBy(int i1, int j1, int i2, int j2) = 0;
        // get distance+1 of two qubits
        virtual int getDistance(int i1, int j1, int i2, int j2) = 0;

        // -------------------------------- output --------------------------------

        void print(std::ostream &os = std::cout);

        void draw(std::ostream &os = std::cout);


        // -------------------------------- others --------------------------------

        /**
         * map one qubit
         * dest: index of the logical qubit
         * i,j: location of the physical qubit
         */
        inline void map(int dest, int i, int j) {
            getQubit(i, j).map_to = dest;
        }

        /**
         * reset the time
         */
        inline void resetTime() {
            for(int i = 0; i < m * n; i++) {
                qubits[i].time = 0;
            }
        }

        /**
         * go on next instruction cycle
         */
        inline void nextCycle() {
            current_time++;            
        };

        /**
         * time when all qubits end up being busy
         */
        inline int maxTime() {
            return qubit_max_time;
        }

        inline int getTime() {
            return current_time;
        }

        /**
         * check if physical qubit on (i, j) free
         */
        inline bool isQubitFree(int i, int j) {
            return getQubit(i, j).time <= current_time;
        }

        /**
         * check if all physical qubits free
         */
        inline bool isAllQubitFree() {
            return qubit_max_time <= current_time;
        }

        inline bool isSimpleGridDevice() {
            return is_simple_grid_device;
        }

    protected:
        bool is_simple_grid_device = false; // can caculate H_fine if it is true
        int m, n;    // m, n: the grid side length
        int current_time;
        int qubit_max_time; // time when all qubits end up being busy
        BasicGridDevice::PhysicalQubit *qubits;
    };


} // namespace CODAR

#endif // BASIC_GRID_DEVICE_H

