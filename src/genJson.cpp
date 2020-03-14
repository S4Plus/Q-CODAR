// generate json file
// a temporary tool, not maked in Makefile
#include <iostream>
#include <vector>

using namespace std;


#define BEGIN_QMAP(M, N)    \
                    const int m = M; \
                    const int n = N; \
                    const bool available_qubits[] = {
#define END_QMAP    };
#define X           true, 
#define _           false, 
#include "google_q54.qmap"
#undef BEGIN_QMAP
#undef END_QMAP
#undef X
#undef _

int *qid;

int cur = 0;
bool met_adj = false;
void beginQubit() {
    cout << "\t\t[ ";
    met_adj = false;
}

void endQubit() {
    cout << " ]," << endl;
    cur++;
}

void printAdj(int i) {
    if(met_adj)
        cout << ", ";
    cout << "{\"v\": \"q[" << qid[i] << "]\"}";
    met_adj = true;
}

void tryPrintAdj(int i) {
    if(i < 0 || i >= m * n)
        return;
    if(available_qubits[i])
        printAdj(i);
}

int main() {
    int size = m * n;
    qid = new int[m * n];
    vector<int> pos;
    int id = 0;
    for(int i = 0; i < size; i++) {
        if(available_qubits[i]) {
            qid[i] = id;
            id++;
            pos.push_back(i);
        }
    }
    for(int i : pos) {
        beginQubit();
        tryPrintAdj(i - n);
        tryPrintAdj(i - 1);
        tryPrintAdj(i + 1);
        tryPrintAdj(i + n);
        endQubit();
    }
    delete [] qid;
}
