#include <exception>
#include <cstring>
#include <cmath>
#include "QScheduler.h"

using namespace CODAR;


// constant symbols
const QScheduler::PhysicsGate::GateType QScheduler::PhysicsGate::CNOT = "cx";
const QScheduler::PhysicsGate::GateType QScheduler::PhysicsGate::SWAP = "swap";


#ifdef TESTING
// global varaibles for debugging
int global_debug_count = 0;
int debug_continuous_swap_count = 0;
#endif


int CODAR::QScheduler::addLogicalQubit(int i, int j) {
    auto next_index = map_list.size();
    map_list.push_back(i * device->getN() + j);
    return next_index;
}

/**
 * The main entry of the CODAR remapper
 */
void CODAR::QScheduler::start() {
    initMapping();  // register mapping to device

    while(!(logical_gate_list.empty() && candidate_gates.empty())) {
        // start with zero priority on device
        // which is guaranteed in the constructor of device
        // reset priorities to 0 after used
        bool deadlock = true;
        selectCandidateGates(); // select CF gates
        DEBUG_DO(std::cerr << "DEBUG: selected " << candidate_gates.size() << " candidate gates:" << std::endl);
        // traverse candidate gates
        for(auto it = candidate_gates.begin(); it != candidate_gates.end(); ) {
            auto &gate = *it;
            if(isImmediateGate(gate)) { // apply it if is an immediate gate
                DEBUG_SHOW("imediate gate", gate);
                if(isAllQubitFree(gate)) {
                    DEBUG_LOG("\tall qubit free");
                    PhysicsGate ph_gate;
                    getPhysicsGate(gate, ph_gate);
                    launch(ph_gate);
#ifdef TESTING
                    sum_min_route_len += gate.min_route_len;
                    assert(gate.min_route_len >= 0);
                    if(gate.isSingleQubitGate())
                        sum_rx_route_len += gate.route_len;
                    else
                        sum_cnot_route_len += gate.route_len;
                    max_route_len = std::max(max_route_len, gate.route_len);
#endif
                    deadlock = false;
                    it = candidate_gates.erase(it);  // erase such element
                    if(it == candidate_gates.end())
                        break;
                    continue;   // `it` is already updated by `it = candidate_gates.erase(it);`
                } else {
                    // immediate gate but can not apply now (not lock free)
                    // participate in calculation of priorities
                    DEBUG_LOG("\tnot all qubit free");
                    if(!gate.isSingleQubitGate()) {
                        // a double-qubit gate may change to a non-immediate gate
                        addPriority(gate);
                    }
                }
            } else {    // non-immediate gate, we can calculate the prioritied and add candidate SWAPs
                // gate must be double qubit gate
                DEBUG_SHOW("far gate", gate);
                addPriority(gate);
            }
            it++;
        }
        DEBUG_PRINT(candidate_gates.size());
        // finished priorities set up
        if(!candidate_swaps.empty()) {
            DEBUG_LOG("candidate swap set not empty");
            // there are candidate swaps
            while(true) {
                DEBUG_LOG("candidate swaps:");
                DEBUG_DO(
                    for(auto &swp : candidate_swaps) {
                        std::cerr << "\t(" << swp.first.first << ", " << swp.first.second << "): " << swp.second.first << std::endl;
                    }
                )

                DEBUG_PRINT(candidate_swaps.size());
                DEBUG_DO(device->draw(std::cerr));
                // select the SWAP with highest priority
                PhysicsGate swap_gate;
                if(!getHighestPrioritySWAP(swap_gate, false)) {
                    // no SWAP selected
                    // check deadlock
                    if(deadlock && device->isAllQubitFree()) {
                        DEBUG_LOG("deadlock occurred");
                        getHighestPrioritySWAP(swap_gate, true);
                        deadlock = false;
                    } else {
                        // no deadlock
                        break;
                    }
                }
                // apply SWAP
                launch(swap_gate);
                deadlock = false;
                // update priorities
                #warning VERY SLOW METHOD USED
                // TODO: I can optimize the method here... to make it faster
                candidate_swaps.clear();
                DEBUG_PRINT(candidate_gates.size());
                for(auto &gate : candidate_gates) {
                    if(!gate.isSingleQubitGate()) {
                        addPriority(gate);
                    }
                }
            }
            // finished routing, clear all priorities
            candidate_swaps.clear();
        }
        DEBUG_LOG("---- NEW CYCLE ----");
        device->nextCycle();
    }
}

inline int getHDVDDiff(int i1, int j1, int i2, int j2) {
    return std::abs(std::abs(i1 - i2) - std::abs(j1 - j2));
}

bool CODAR::QScheduler::getHighestPrioritySWAP(PhysicsGate &gate, bool deadlock) {
    std::pair<int,int> max_priority(deadlock ? -1 : 0, __INT_MAX__);
    int best_i1 = -1, best_j1 = -1, best_i2 = -1, best_j2 = -1;
    for(auto it = candidate_swaps.begin(); it != candidate_swaps.end(); it++) {
        auto &swp = it->first;
        auto &priority = it->second;
        // DEBUG_DO(std::cerr << "DEBUG: check priority of swap " << swp.first << ", " << swp.second << ": " << priority.first << ", " << priority.second << std::endl);
        int i1 = swp.first / device->getN();
        int j1 = swp.first % device->getN();
        int i2 = swp.second / device->getN();
        int j2 = swp.second % device->getN();
        assert(device->isNearBy(i1, j1, i2, j2));
        // DEBUG_PRINT(device->getQubit(i1, j1).time);
        // DEBUG_PRINT(device->getQubit(i2, j2).time);
        // DEBUG_PRINT(device->getTime());
        if(priority > max_priority && device->canSwap(i1, j1, i2, j2)) {
            // DEBUG_LOG("set as best swap, continue...");
            best_i1 = i1;
            best_j1 = j1;
            best_i2 = i2;
            best_j2 = j2;
            max_priority = priority;
        }
    }
    if(max_priority.first <= 0) {
        if(!deadlock)
            return false;
        /*
        if(max_priority.first != 0) {
            for(auto &cg : candidate_gates) {
                std::cerr << cg << std::endl;
            }
            device->draw(std::cerr);
        }
        */
        assert(max_priority.first == 0);
    }
    gate.type = gate.SWAP;
    gate.i1 = best_i1;
    gate.j1 = best_j1;
    gate.i2 = best_i2;
    gate.j2 = best_j2;
    return true;
}

void CODAR::QScheduler::addPriority(const LogicalGate &gate) {
    DEBUG_SHOW("add priority for gate", gate.gate);
    assert(!gate.isSingleQubitGate());
    int i1, j1, i2, j2;     // pos of qubits of gate
    getMappedPosition(gate.c, i1, j1);
    getMappedPosition(gate.t, i2, j2);
    int dist0 = device->getDistance(i1, j1, i2, j2);    // distance of qubits of gate
    int hv_diff = 0;        // hd-vd difference
    if(device->isSimpleGridDevice()) {
        hv_diff = getHDVDDiff(i1, j1, i2, j2);
    }
    for(auto &q : device->getQubit(i1, j1).nearbyQubits) {
        if(q.first == i2 && q.second == j2)
            continue;
        int dist = device->getDistance(q.first, q.second, i2, j2);
        int fine = 0;
        if(device->isSimpleGridDevice()) {
            fine = hv_diff - getHDVDDiff(q.first, q.second, i2, j2);
        }
        addPriorityForSwap(q.first, q.second, i1, j1, dist0 - dist, fine);
    }
    for(auto &q : device->getQubit(i2, j2).nearbyQubits) {
        if(q.first == i1 && q.second == j1)
            continue;
        int dist = device->getDistance(i1, j1, q.first, q.second);
        int fine = 0;
        if(device->isSimpleGridDevice()) {
            fine = hv_diff - getHDVDDiff(i1, j1, q.first, q.second);
        }
        addPriorityForSwap(i2, j2, q.first, q.second, dist0 - dist, fine);
    }
}

// list of know gates
#define DEF_GATE2(G)    G##1, G##2,
#define DEF_GATE(G)     G,
#define ALIAS_GATE(ALS, G)
#define COMMUTE(G1, G2)
    enum gate_enum {
        none,
#include "commuting.def"
        other
    };
#undef DEF_GATE2
#define DEF_GATE2(G)
#undef DEF_GATE
#define DEF_GATE(G)

static unsigned int commuting_table[gate_enum::other + 1];

void CODAR::loadCommutingTable() {
    commuting_table[gate_enum::none] = 0xFFFFFFFF;
    for(int i = 1; i < gate_enum::other; i++) {
        commuting_table[i] = 1 << i;
    }
#undef COMMUTE
#define COMMUTE(G1, G2) commuting_table[gate_enum::G1] |= 1 << gate_enum::G2;
#include "commuting.def"
#undef COMMUTE
#define COMMUTE(G1, G2)
    commuting_table[gate_enum::other] = 0;
}

void getGtGc(CODAR::QScheduler::LogicalGate &logical_gate, gate_enum &gt, gate_enum &gc) {
    // toupper
    char gate[16];
    for(int i = 0; i < 16; i++) {
        char c = logical_gate.gate[i];
        if(c == 0 || c == '(') {
            gate[i] = 0;
            break;
        }
        gate[i] = std::toupper(c);
    }
    gt = gate_enum::other;
    gc = gate_enum::none;
    const char *pgate = gate;

#undef ALIAS_GATE
#define ALIAS_GATE(ALS, G)  if(strcmp(gate, #ALS) == 0) { pgate = #G; }
#include "commuting.def"
#undef ALIAS_GATE
#define ALIAS_GATE(ALS, G)

#undef DEF_GATE2
#undef DEF_GATE
#define DEF_GATE2(G)        if(strcmp(pgate, #G) == 0) { gt = gate_enum::G##2; gc = gate_enum::G##1; }
#define DEF_GATE(G)         if(strcmp(pgate, #G) == 0) { gt = gate_enum::G; }
#include "commuting.def"
#undef DEF_GATE2
#undef DEF_GATE
#undef ALIAS_GATE
#undef COMMUTE
}


void QScheduler::selectCandidateGates() {
    size_t qcount = getLogicalQubitCount();
    unsigned char *state = new unsigned char[qcount];
    memset(state, 0, qcount);

    for(auto &logical_gate : candidate_gates) {
        gate_enum gt, gc;
        getGtGc(logical_gate, gt, gc);
        state[logical_gate.t] |= 1 << gt;
        if(logical_gate.c >= 0)            
            state[logical_gate.c] |= 1 << gc;            
    }

    int _temp_count = 0;
    for(auto it = logical_gate_list.begin(); it != logical_gate_list.end();) {
        auto &logical_gate = *it;
        gate_enum gt, gc;
        getGtGc(logical_gate, gt, gc);
        // now，gt and gc is the type of the gates
        if(
            (~(commuting_table[gt]) & (state[logical_gate.t])) == 0 &&
            (logical_gate.c < 0 || (~(commuting_table[gc]) & (state[logical_gate.c])) == 0)
        ) {
            // commuting
            // insert it into candidate_gates
#ifdef TESTING
            if(!it->isSingleQubitGate()) {
                int i1, j1, i2, j2;
                getMappedPosition(it->c, i1, j1);
                getMappedPosition(it->t, i2, j2);
                it->min_route_len = std::abs(i1 - i2) + std::abs(j1 - j2) - 1;
            }
#endif
            candidate_gates.splice(candidate_gates.end(), logical_gate_list, it++);
            _temp_count = 0;
        } else {
            ++it;
            ++_temp_count;
            if(_temp_count > device->getM() * device->getN() * 4)
                break;
        }
        state[logical_gate.t] |= 1 << gt;            
        if(logical_gate.c >= 0)            
            state[logical_gate.c] |= 1 << gc;
    }

    delete[] state;
}


void QScheduler::launch(QScheduler::PhysicsGate &gate) {
#ifdef TESTING
    global_debug_count++;
    if(gate.isSwapGate()) {
        debug_continuous_swap_count++;
        if(debug_continuous_swap_count > 20) {
            std::cerr << "!!! WARNING: " << global_debug_count << std::endl;
            assert(false);
        }
    } else {
        debug_continuous_swap_count = 0;
    }
#endif

    DEBUG_SHOW("lanching gate", gate);
    gate_count++;
    outputGateSymbol(gate);
    outputQubit(gate.i1, gate.j1);
    if(gate.isSwapGate()) {
        // update the map table
        int q1 = device->getQubit(gate.i1, gate.j1).map_to;
        int q2 = device->getQubit(gate.i2, gate.j2).map_to;
        if(q1 != -1)
            map_list[q1] = gate.i2 * device->getN() + gate.j2;
        if(q2 != -1)
            map_list[q2] = gate.i1 * device->getN() + gate.j1;
        out << ", ";
        outputQubit(gate.i2, gate.j2);
#ifdef TESTING
        for(auto &gate : candidate_gates) {
            if(
                (gate.c != -1 && (gate.c == q1 || gate.c == q2)) ||
                (gate.t == q1 || gate.t == q2)
            ) {
                gate.route_len++;
            }
        }
#endif
        // operate on device
        device->applySwap(gate.i1, gate.j1, gate.i2, gate.j2);
    } else if(gate.isControlGate()) {
        int i, j;
        gate.getSecondQubitPostion(i, j);
        out << ", ";
        outputQubit(i, j);
        device->applyCNot(gate.i1, gate.j1, gate.i2, gate.j2);
    } else {
        device->applyRx(gate.i1, gate.j1);
    }
    endOutputGate();
    DEBUG_PAUSE();
}

bool CODAR::QScheduler::addLogicalQubits(int count) {
    // create a random sequence
    srand(time(nullptr));
    int *random_map = new int[count];
    for(int i = 0; i < count; i++) {
        random_map[i] = i;
    }
    for(int i = count - 1; i > 0; i--) {
        std::swap(random_map[i], random_map[rand() % i]);
    }

    int m = device->getM();
    int n = device->getN();
    int mid_i = m / 2, mid_j = n / 2;
    if(m * n < count) {
        return false;
    }
    // try to map in a square
    int a = std::ceil(std::sqrt(count));
    if(a > m) {
        n = std::ceil((float)count / m);
    } else if(a > n) {
        m = std::ceil((float)count / n);
    } else {
        m = n = a;
    }
    int i0 = mid_i - m / 2;
    int j0 = mid_j - n / 2;
    for(int k = 0; k < count; k++) {
        int mp = random_map[k];
        int i = i0 + mp / n;
        int j = j0 + mp % n;
        addLogicalQubit(i, j);
    }
    delete[] random_map;
    return true;
}