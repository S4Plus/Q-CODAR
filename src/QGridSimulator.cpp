/**
 * Simulate a qasm program which has well remapped
 * Use for debugging.
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include "IdealGridDevice.h"
#include "QASMParser.h"

using std::cin;
using std::endl;
using std::cerr;
using std::cout;
using std::string;
using namespace CODAR;


void readInitMapping(int *map_list, int qcount, int max_qcount, std::istream &is = cin) {
    assert(qcount <= max_qcount);
    char c1, c2, c3;
    int from, to;
    for(int i = 0; i < qcount; i++) {
        if(!(is >> from >> c1 >> c2 >> to >> c3)) {
            cerr << "read init mapping error" << endl;
            throw;
        }
        assert(from == i);
        assert(c1 == '=');
        assert(c2 == '>');
        assert(c3 == ';');
        assert(to >= 0 && to < max_qcount);
        map_list[from] = to;
    }
}


int main(int argc, char **argv) {
    int m, n;
    m = DEVICE_M;
    n = DEVICE_N;
    int input_qcount = 0;
    if(argc < 2) {
        cerr << "usage: " << argv[0] << " filename" << endl;
        return 0;
    }
    for(int i = 2; i < argc; i++) {
        if(strcmp(argv[i], "-q") == 0) {
            if(i + 1 < argc) {
                input_qcount = atoi(argv[i + 1]);
            } else {
                cerr << "need qubit count after '-q'" << endl;
            }
        }
        if(strcmp(argv[i], "-m") == 0) {
            if(i + 1 < argc) {
                m = atoi(argv[i + 1]);
            } else {
                cerr << "need qubit count after '-m'" << endl;
            }
        }
        if(strcmp(argv[i], "-n") == 0) {
            if(i + 1 < argc) {
                n = atoi(argv[i + 1]);
            } else {
                cerr << "need qubit count after '-n'" << endl;
            }
        }
    }
    auto is = std::ifstream(argv[1]);

    std::ostringstream output;

    QASMParser parser(is);
    IdealGridDevice device(m, n);
    
    try {
        int qcount = parser.getQubitCount();
        int *map_list = new int[qcount];
        for(int i = 0; i < qcount; i++) {
            map_list[i] = -1;
        }
        auto map_filename = string(argv[1]) + ".map";
        int used_qcount = qcount;
        #ifdef QCOUNT
            used_qcount = QCOUNT;
        #endif
        if(input_qcount > 0) {
            used_qcount = input_qcount;
        }
        if(access(map_filename.c_str(), R_OK) == 0) {
            auto map_is = std::ifstream(map_filename);
            readInitMapping(map_list, used_qcount, m * n, map_is);
        } else {
            cout << "未找到初始映射文件" << endl;
            cout << "请输入初始映射：" << endl;
            readInitMapping(map_list, used_qcount, m * n);
        }
        for(int i = 0; i < qcount; i++) {
            int v = map_list[i];
            if(v < 0)
                continue;
            device.getQubit(v / n, v % n).map_to = i;
        }

        device.draw();

        while(true) {
            int q1, q2;
            const char *gate = parser.getInstruction(q1, q2);
            assert(q1 < qcount && q1 >= 0);
            assert(q2 < qcount && q2 >= -1);
            int i1, j1;
            i1 = q1 / n;
            j1 = q1 % n;
            int i2, j2;
            if(q2 != -1) {
                i2 = q2 / n;
                j2 = q2 % n;
            }
            while(!(device.isQubitFree(i1, j1) && (q2 == -1 || device.isQubitFree(i2, j2)))) {
                cout << "---- NEXT CYCLE ----" << endl;
                device.nextCycle();
            }
            if(gate == nullptr)
                break;
            if(q2 == -1) {
                int t = device.getQubit(i1, j1).time;
                device.applyRx(i1, j1);
                cout << t << ":\t" << gate << " (" << i1 << ", " << j1 << ")" << endl;
                string gstr(gate);
                int bra_pos = gstr.find_first_of('(');
                cout << "\t--> " << gstr.substr(0, bra_pos) << " " << device.getQubit(i1, j1).map_to << endl;
            } else {
                int t = std::max(device.getQubit(i1, j1).time, device.getQubit(i2, j2).time);
                if(
                    strcmp(gate, "swap") == 0 || 
                    strcmp(gate, "intrinsic_swap__") == 0
                ) {
                    device.applySwap(i1, j1, i2, j2);
                }
                else {
                    device.applyCNot(i1, j1, i2, j2);
                }
                cout << t << ":\t" << gate << " (" << i1 << ", " << j1 << ") (" << i2 << ", " << j2 << ")" << endl;
                string gstr(gate);
                if(gstr == "intrinsic_swap__")
                    gstr = "swap";
                int bra_pos = gstr.find_first_of('(');
                cout << "\t--> " << gstr.substr(0, bra_pos) << " " << device.getQubit(i1, j1).map_to<< ",  " << device.getQubit(i2, j2).map_to << endl;
                if(gstr == "swap") {
                    device.draw();
                }
            }
            while(cin.get() != '\n');
        }

    } catch (const std::exception &e) {
        cerr << "error: " << e.what() << endl;
        cerr << "\tat: " << parser.getCurrentLine() << endl;
        return -1;
    }
    cerr << endl;
    return 0;
}