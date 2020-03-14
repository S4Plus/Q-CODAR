/**
 * QScheduler.h
*/

#ifndef QSCHEDULER_H
#define QSCHEDULER_H

#include <list>
#include <vector>
#include <set>
#include <map>
#include "BasicGridDevice.h"


namespace CODAR {

    void loadCommutingTable(void);

    class QScheduler {
    public:
        // out: output stream for output the generated circuit
        inline QScheduler(BasicGridDevice *device, std::ostream &out = std::cout)
            : device(device), out(out) {
            // Nothing to do
        }


        // ----------------------------------------------------------------
        // Gate struct definition

        /**
         * logical gate
         */
        struct LogicalGate {
            int t;  // index of the target qubit
            int c;  // index of the control qubit, c = -1 for single qubit gate
            const char *gate;   // type of the gate
#ifdef TESTING
            int min_route_len = 0;      // min route length, the distance of qubits when it inserted into candidate gate list
            int route_len = 0;          // the actual route length
#endif
            bool isSingleQubitGate() const {
                return c == -1;
            }
            /**
             *  create a single-qubit gate
             */
            inline LogicalGate(const char *gate, int t)
                :t(t), gate(gate) { c = -1; }
            /**
             *  create a double-qubit gate
             */
            inline LogicalGate(const char *gate, int t, int c)
                :t(t), c(c), gate(gate) { }

            inline friend std::ostream &operator<<(std::ostream &os, LogicalGate &gate) {
                return gate.isSingleQubitGate() ?
                    os << gate.gate << "(" << gate.t << ")"
                    :
                    os << gate.gate << "(" << gate.c << ", " << gate.t << ")"
                    ;
            }

            /*
            inline bool operator<(LogicalGate &right)& {
                return t < right.t || (t == right.t && c < right.c);
            }
            */
        };

        /**
         * physical gate
         */
        struct PhysicsGate {
            // GateType: all charactors should be capital
            typedef const char *GateType;
            GateType type;
            // position of qubits
            int i1, j1;     // (i1, j1) is the position for the first qubit parament of the gate
            int i2, j2;     // (i2, j2) is the position for the first qubit parament of the two-qubit gate

            static const GateType CNOT;
            static const GateType SWAP;

            inline bool isSwapGate() {
                return type == SWAP;
            }

            inline bool isControlGate() {
                return type[0] == 'c';
            }

            /**
             * get position of the first qubit
             */
            inline void getFirstQubitPosition(int &i, int &j) const {
                i = this->i1;
                j = this->j1;
            }

            /**
             * get position of the second qubit for 2-qubit gates
             */
            inline void getSecondQubitPostion(int &i, int &j) const {
                i = i2;
                j = j2;
            }

            inline friend std::ostream &operator<<(std::ostream &os, PhysicsGate &gate) {
                if(gate.isControlGate() || gate.isSwapGate()) {
                    return os << "physics " << gate.type << " (" << gate.i1 << ", " << gate.j1 << ") "
                        << " (" << gate.i2 << ", " << gate.j2 << ")";
                } else {
                    return os << "physics " << gate.type << " (" << gate.i1 << ", " << gate.j1 << ")";
                }
                        
            }

        };



        // ----------------------------------------------------------------
        // get properties

        /**
         * get count of logical qubits
         */
        inline size_t getLogicalQubitCount() {
            return map_list.size();
        }

        


        // ----------------------------------------------------------------
        // functions below are about circuit initialzation

        /**
         * add a logical qubit by its position
         * return the index of the logical qubit
         */
        int addLogicalQubit(int i, int j);

        /**
         * add several logical qubits and make initial mapping automaticly
         * count: count of qubits
         * return true if success
         * MUST ENSURE YOU HAVEN"T MAPPED ANY QUBIT BEFORE CALLING
         */
        bool addLogicalQubits(int count);

        /**
         * add a (logical) single qubit gate
         * i: index of the logical qubit
         */
        inline void addSingleQubitGate(const char *gate, int i) {
            /**
             * TODO:
             * Complex gate decomposition
             * maybe not necessary
             * std::vector<PhysicsGate::GateType> gates = decompositionComplexGate(gate);
             * for(auto &g : gates) {
             *      logical_gate_list.emplace_back(g, i);
             * }
            */
            logical_gate_list.emplace_back(gate, i);
        }

        /**
         * add a logical CNOT gate
         * t: index of the target qubit
         * c: index of the control qubit
        */
        inline void addCNotGate(const char *gate, int c, int t) {
            logical_gate_list.emplace_back(gate, t, c);
        }

        // TODO
        // void addMeasureTask(int i);

        // ----------------------------------------------------------------
        // actions

        /**
         * start remapping (call it after intialization)
         */
        void start();


    private:
        /**
         * create the candidate gate set
         */
        void selectCandidateGates();

        /**
         * get a SWAP with highest priority in all possible routing SWAPs which could be apply in parallel now,
         * return false if no SWAP with positive priority found
         * if deadlock == true, SWAP with zero priority may be ok, too.
         */
        bool getHighestPrioritySWAP(PhysicsGate &gate, bool deadlock);

        /**
         * launch a physical gate (to the device), change the statement and output
         */
        void launch(PhysicsGate &gate);

        /**
         * call this function after input initial mapping.
         * this function will initialize the device
         * register mapping infomation to device
         * may cause error if you call the function for more than once or the initial mapping has conflicts
         * must ensure the mapping infomation in the device is empty
         */
        void initMapping(void) {
            for(size_t k = 0; k < getLogicalQubitCount(); k++) {
                int i, j;
                this->getMappedPosition(k, i, j);
                auto &q = device->getQubit(i, j).map_to;
                assert(q == -1);
                q = k;
            }
            gate_count = 0;
            sum_min_route_len = 0;
            sum_rx_route_len = 0;
            sum_cnot_route_len = 0;
            max_route_len = 0;
        }

        // ----------------------------------------------------------------
        // get infomation

        /**
         * get the physical position of the logical qubit
         * logical: index of the logical qubit
         * i，j: position of the physical qubit
         */
        inline void getMappedPosition(int logical, int &i, int &j) {
            int x = map_list[logical];
            i = x / device->getN();
            j = x % device->getN();
        }

        /**
         * check whether a gate is immediate gate or not
         */
        inline bool isImmediateGate(LogicalGate &gate) {
            if(gate.isSingleQubitGate()) {
                // assume all 1-qubit gates are immediate
                return true;
            } else {
                int i1, j1, i2, j2;
                getMappedPosition(gate.c, i1, j1);
                getMappedPosition(gate.t, i2, j2);
                return device->isNearBy(i1, j1, i2, j2);
            }
        }

        /**
         * check whether the logical qubit is free or not
         */
        inline bool isLogicalQubitFree(int logical_qubit) {
            // DEBUG_SHOW("testing logical qubit free", logical_qubit);
            int i, j;
            getMappedPosition(logical_qubit, i, j);
            // DEBUG_PRINT(device->getQubit(i, j).time);
            // DEBUG_PRINT(device->getTime());
            // DEBUG_PRINT(device->isQubitFree(i, j));
            return device->isQubitFree(i, j);
        }

        /**
         * check whether all qubits of the logical gate are free or not
         */
        inline bool isAllQubitFree(LogicalGate &gate) {
            return isLogicalQubitFree(gate.t) && (
                gate.isSingleQubitGate() || isLogicalQubitFree(gate.c)
            );
        }

        /**
         * get the corresponding physical gate of a logical gate
         */
        inline void getPhysicsGate(LogicalGate &logical_gate, PhysicsGate &physics_gate) {
            int it, jt, ic, jc;
            physics_gate.type = logical_gate.gate;
            getMappedPosition(logical_gate.t, it, jt);
            if(logical_gate.isSingleQubitGate()) {
                physics_gate.i1 = it;
                physics_gate.j1 = jt;
            } else {
                getMappedPosition(logical_gate.c, ic, jc);
                physics_gate.i1 = ic;
                physics_gate.j1 = jc;
                physics_gate.i2 = it;
                physics_gate.j2 = jt;
            }
        }

        /**
         * update the priority (both H_main and H_fine) of a SWAP by a logical gate
         * if the SWAP is not in candidate SWAP list, insert it into at first.
         */
        void addPriority(const LogicalGate &gate);

        inline void addPriorityForSwap(int i1, int j1, int i2, int j2, int main, int fine) {
            DEBUG_DO(std::cerr << "DEBUG: priority of (" << i1 << ", " << j1 << ") : (" << i2 << ", " << j2 << ") += " << main << std::endl);
            int q1 = i1 * device->getN() + j1;
            int q2 = i2 * device->getN() + j2;
            if(q1 > q2) {
                std::swap(q1, q2);
            }
            auto swp = std::pair<int,int>(q1, q2);
            if(candidate_swaps.count(swp) == 0) {
                candidate_swaps[swp] = std::pair<int,int>(main, fine);
            } else {
                candidate_swaps[swp].first += main;
                candidate_swaps[swp].second += fine;
            }
        }

        // ----------------------------------------------------------------
        // output functions

        /**
         * output a qubit
         */
        inline void outputQubit(int i, int j) {
#ifdef OUTPUT_RESULT
            out << "q[" << i * device->getN() + j << "]";
#endif
        }

        /**
         * output a symbol of a gate, e.g. "swap"
         */
        inline void outputGateSymbol(PhysicsGate &gate) {
#ifdef OUTPUT_RESULT
            out << gate.type << " ";
#endif
        }

        /**
         * finish to output a gate
         */
        inline void endOutputGate() {
#ifdef OUTPUT_RESULT
            out << ";" << std::endl;
#endif
        }

    public:
        std::vector<int> map_list;                  // the mapping table from logical qubits to physical qubits, see getMappedPosition
        BasicGridDevice *device;                    // the device
        std::ostream &out;                          // the output stream
        std::list<LogicalGate> logical_gate_list;   // the list of logical gates (the whole circuit)
        std::list<LogicalGate> candidate_gates;     // the set of Commutative Forward gates
        int gate_count = 0;                         // count of gates launched

        // q1, q2: physical qubits to SWAP
        // candidate_swaps[std::pair(q1, q2)] = std::pair(H_main, H_fine)
        // maps from candidate swaps to their heuristic costs
        std::map<std::pair<int, int>, std::pair<int, int>>  candidate_swaps;    
#ifdef TESTING
        int sum_min_route_len = 0;
        int sum_rx_route_len = 0;
        int sum_cnot_route_len = 0;
        int max_route_len = 0;
#endif

    };

} // CODAR

#endif // QSCHEDULER_H