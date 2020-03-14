#include "BasicGridDevice.h"

using namespace CODAR;

CODAR::BasicGridDevice::BasicGridDevice(int m, int n)
    : m(m), n(n) {
    assert(m > 0);
    assert(n > 0);
    qubits = new PhysicalQubit[m * n];
    clear();
}

void CODAR::BasicGridDevice::clear() {
    for(int i = 0; i < m * n; i++) {
        qubits[i].map_to = -1;
    }
    current_time = 0;
    qubit_max_time = 0;
    resetTime();
}

CODAR::BasicGridDevice::~BasicGridDevice() {
    delete[] qubits;
}

void CODAR::BasicGridDevice::print(std::ostream &os) {
    for(int i = 0; i < m; i++) {
        for(int j = 0; j < n; j++) {
            auto &q = getQubit(i, j);
            if(q.map_to != -1) {
                os << "( " << i << ", " << j << "): "
                    << q.map_to << std::endl;
            }
        }
    }
}

void CODAR::BasicGridDevice::draw(std::ostream &os) {
#define DRAW_ON_QUBIT map_to
    for(int i = 0; i < m; i++) {
        for(int j = 0; j < n; j++) {
            int x = getQubit(i, j).DRAW_ON_QUBIT;
            if(x == -1)
                os << "-+-";
            else
                os << '[' << x << ']';
            if(j == n - 1) {
                os << std::endl;
                break;
            }
            os << "-----";
        }
        if(i == m - 1)
            break;
        for(int j = 0; j < n - 1; j++) {
            os << " |      ";
        }
        os << " | " << std::endl;
        for(int j = 0; j < n - 1; j++) {
            os << " |      ";
        }
        os << " | " << std::endl;
        for(int j = 0; j < n - 1; j++) {
            os << " |      ";
        }
        os << " | " << std::endl;
    }
#undef DRAW_ON_QUBIT
}