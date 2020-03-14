#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <stack>
#include <unistd.h>
#include <fcntl.h>
#include <queue>
#include "QScheduler.h"
#include "QASMParser.h"
#include "SimpleGridDevice.h"
#include "ExGridDevice.h"
#include "UncompletedGridDevice.h"

using std::cin;
using std::endl;
using std::cerr;
using std::cout;
using std::string;
using namespace CODAR;

struct inst_t {
    const char *gate;
    int q1, q2;
};


void readInitMapping(int *map_list, int qcount, int max_qcount, std::istream &is = cin) {
#ifdef QCOUNT
    qcount = QCOUNT;
#endif
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


void printHelp(const char *arg0) {
    cerr << "usage: " << arg0 << " <filename> [args]" << endl;
    cerr << "args:" << endl;
    cerr << "\t-o <out_file>   default: " << "output/*.qasm" << "\tset output file name" << endl;
    cerr << "\t-m <M>          default: " << DEVICE_M << "      \tset side length for grid device" << endl;
    cerr << "\t-n <N>          default: " << DEVICE_N << "      \tset side length for grid device" << endl;
    cerr << "\t-r <run_time>   default: " << RUN_TIME << "      \trun <run_time> times and output the best one" << endl;
    cerr << "\t-init <file>                            use a specific initial mapping from file" << endl;
    cerr << "\t--init                                  use a specific initial mapping from stdin" << endl;
    cerr << "\t--tokyo                                 use ibm-tokyo 20-qubit device model" << endl;
    cerr << "\t--ibm_q53                               use ibm-Rochester 53-qubit device model" << endl;
    cerr << "\t--google_q54                            use google-Sycamore 54-qubit device model" << endl;
}

int main(int argc, char **argv) {
    if(argc < 2) {
        printHelp(argv[0]);
        return 0;
    }
    auto is = std::ifstream(argv[1]);

    int m = DEVICE_M;
    int n = DEVICE_N;
    int run_time = RUN_TIME;
    const char *outfilename = nullptr;
    const char *initial_mapping_filename = nullptr;
    bool initial_mapping_from_stdin = false;
    bool use_tokyo = false;
    bool use_ibm_q53 = false;
    bool use_google_q54 = false;
    bool a_star_init_mapping = false;
#define GET_IARG(NAME, VALUE) \
    if(strcmp(argv[i], "-" #NAME) == 0) {\
        if(i + 1 >= argc) { cerr << "Need value for " #NAME << endl; return -1; } \
        VALUE = atoi(argv[i + 1]); \
        i++; \
    }

#define GET_SARG(NAME, VALUE) \
    if(strcmp(argv[i], "-" #NAME) == 0) {\
        if(i + 1 >= argc) { cerr << "Need value for " #NAME << endl; return -1; } \
        VALUE = argv[i + 1]; \
        i++; \
    }

#define GET_FLAG(NAME, VALUE) \
    if(strcmp(argv[i], "--" #NAME) == 0) {\
        VALUE = true;   \
    }

    for(int i = 2; i < argc; i++) {
        GET_IARG(m, m);
        GET_IARG(n, n);
        GET_IARG(r, run_time);
        GET_SARG(o, outfilename);
        GET_SARG(init, initial_mapping_filename);
        GET_FLAG(init, initial_mapping_from_stdin);
        GET_FLAG(tokyo, use_tokyo);
        GET_FLAG(ibm_q53, use_ibm_q53);
        GET_FLAG(google_q54, use_google_q54);
        GET_FLAG(a_star_init, a_star_init_mapping);
    }

    if(a_star_init_mapping && run_time != 1) {
        cerr << "warning: you use a* method for initial mapping but set run_time (-r) as " << run_time << endl;
    }

    loadCommutingTable();

    std::ostringstream output;

    BasicGridDevice *device;

    #define QPAIR(Q1, Q2)   lines.emplace_back(Q1, Q2);
    #define QPAIRS(Q1, Q2)  QPAIR(Q1, Q2) QPAIR(Q2, Q1)
    if(use_tokyo) {
        std::cerr << "use ibm tokyo model" << std::endl;
        std::vector<std::pair<int, int>> lines;
        #include "../arch/qpair/tokyo.qpair"
        device = new ExGridDevice(m, n, lines);
        // assert(device->isNearBy(1, 3, 2, 2));
    } else if(use_ibm_q53) {
        std::cerr << "use ibm q53 model" << std::endl;
        std::vector<std::pair<int, int>> lines;
        #include "../arch/qpair/ibm_q53.qpair"
        device = new ExGridDevice(m, n, lines);
    } else if(use_google_q54) {
        std::cerr << "use google q54 model" << std::endl;
        #define BEGIN_QMAP(M, N) \
                            m = M; n = N; \
                            const bool available_qubits[] = {
        #define END_QMAP    };
        #define X           true, 
        #define _           false, 
        #include "google_q54.qmap"
        #undef BEGIN_QMAP
        #undef END_QMAP
        #undef X
        #undef _
        device = new UncompletedGridDevice(m, n, available_qubits);
    } else {
        // std::cerr << "use " << m << "x" << n << " grid model" << std::endl;
        device = new SimpleGridDevice(m, n);
    }
    QScheduler scheduler(device, output);
    QASMParser parser(is);

    const char *filename = argv[1];
    const char *p = filename;
    while(*p) {
        if(*(p++) == '/') {
            filename = p;
        }
    }

    // cerr << "DEBUG: initialized" << endl;

    try {
        int qcount = parser.getQubitCount();

        int max_q = -1;
        std::vector<inst_t> inst_vec;
        // read instruments
        while(true) {
            int q1, q2;
            const char *gate = parser.getInstruction(q1, q2);
            if(gate == nullptr)
                break;
            max_q = std::max(max_q, q1);
            max_q = std::max(max_q, q2);
            inst_t inst = {gate, q1, q2};
            inst_vec.push_back(inst);
        }
        assert(max_q >= 0 && max_q < qcount);
        qcount = max_q + 1;
        if(initial_mapping_filename != nullptr || initial_mapping_from_stdin) {
            int *init_map_list = new int[qcount];
            for(int i = 0; i < qcount; i++) {
                init_map_list[i] = -1;
            }
            if(initial_mapping_filename && access(initial_mapping_filename, R_OK) == 0) {
                if(initial_mapping_from_stdin) {
                    cerr << "error: you choose initial mapping both from stdin and file." << endl;
                }
                auto map_is = std::ifstream(initial_mapping_filename);
                readInitMapping(init_map_list, qcount, m * n, map_is);
            } else {
                if(initial_mapping_filename) {
                    cout << "initial mapping file " << initial_mapping_filename << " not found" << endl;
                }
                cout << "please input initial mapping:" << endl;
                readInitMapping(init_map_list, qcount, m * n);
            }
            for(int k = 0; k < qcount; k++) {
                int i = init_map_list[k] / n;
                int j = init_map_list[k] % n;
                scheduler.addLogicalQubit(i, j);
            }
        } else if(a_star_init_mapping) {
            cerr << "A star initial mapping method not implemented yet" << endl;
            exit(-1);
        } else if(use_google_q54) {
            // TODO
            // UncompletedGridDevice
            assert(qcount <= 53);
            /*
                0 1 2 3 4 5 6 7 8 9
            0   _ _ _ _ X _ _ _ _ _ 0
            1   _ _ _ X X X _ _ _ _ 1
            2   _ _ X X X X X _ _ _ 2
            3   _ X X X X X X _ _ _ 3
            4   X X X X X X X X X _ 4
            5   _ X X X X X X X X X 5
            6   _ _ X X X X X X X X 6
            7   _ _ _ X X X X X X _ 7
            8   _ _ _ _ X X X X _ _ 8
            9   _ _ _ _ _ X X _ _ _ 9
                0 1 2 3 4 5 6 7 8 9
            *//*===================*//*
                0 1 2 3 4 5 6 7 8 9
            0   _ _ _ _ D _ _ _ _ _ 0
            1   _ _ _ A B C _ _ _ _ 1
            2   _ _ W V U T S _ _ _ 2
            3   _ 7 X C B A 9 _ _ _ 3
            4   E 8 Y D 2 1 8 N O _ 4
            5   _ 9 Z E 3 0 7 M P 5 5
            6   _ _ 0 F 4 5 6 L Q 6 6
            7   _ _ _ G H I J K R _ 7
            8   _ _ _ _ 1 2 3 4 _ _ 8
            9   _ _ _ _ _ F G _ _ _ 9
                0 1 2 3 4 5 6 7 8 9
            */
            int ijlist[] = {
                5, 5,
                4, 5,
                4, 4,
                5, 4,
                6, 4,
                6, 5,
                6, 6,
                5, 6,
                4, 6,
                3, 6,
                3, 5,
                3, 4,
                3, 3,
                4, 3,
                5, 3,
                6, 3,
                7, 3,
                7, 4,
                7, 5, 
                7, 6, 
                7, 7, 
                6, 7, 
                5, 7, 
                4, 7, 
                4, 8,
                5, 8,
                6, 8,
                7, 8,
                2, 6,
                2, 5,
                2, 4,
                2, 3,
                2, 2,
                3, 2,
                4, 2, 
                5, 2,
                6, 2,
                8, 4,
                8, 5,
                8, 6, 
                8, 7,
                5, 9,
                6, 9,
                3, 1,
                4, 1,
                5, 1,
                1, 3,
                1, 4, 
                1, 5,
                0, 4,
                4, 0,
                9, 5,
                9, 6
            };
            srand(time(nullptr));
            int *random_map = new int[qcount];
            for(int i = 0; i < qcount; i++) {
                random_map[i] = i;
            }
            for(int i = qcount - 1; i > 0; i--) {
                std::swap(random_map[i], random_map[rand() % i]);
            }

            for(int q = 0; q < qcount; q++) {
                int qq = random_map[q];
                scheduler.addLogicalQubit(ijlist[2 * qq], ijlist[2 * qq + 1]);
            }
        } else {
            scheduler.addLogicalQubits(qcount);
        }

#ifdef RESV_FIRST
        for(auto it = inst_vec.cbegin(); it != inst_vec.cend(); it++) {
#else
        for(auto it = inst_vec.begin(); it != inst_vec.end(); it++) {
#endif
            auto gate = it->gate;
            auto q1 = it->q1;
            auto q2 = it->q2;
            if(q2 == -1) {
                scheduler.addSingleQubitGate(gate, q1);
            } else {
                scheduler.addCNotGate(gate, q1, q2);
            }
        }

        // cerr << "DEBUG: file loaded" << endl;

        // save to inst_list
        auto inst_list = scheduler.logical_gate_list;
        // 
        auto stored_map_list = scheduler.map_list;
        auto best_map_list = stored_map_list;
        int best_max_time = __INT_MAX__;
        int gate_count = 0;
        int sum_min_route_len = 0, sum_cnot_route_len = 0, sum_rx_route_len = 0, max_route_len = 0;
        bool best_dir = true;
        int which_best = -1;
        bool dir = 
#ifdef RESV_FIRST
        false;
#else
        true;
#endif
        string best_output;
        // start running codar
        // cerr << "DEBUG: start running" << endl;
        for(int i = 0; i < run_time; i++) {
            scheduler.start();
            int max_time = device->maxTime();
            /*
            cerr << "max time = " << max_time
                << " gate count = " << scheduler.gate_count
                << endl;
            */
            if(max_time <= best_max_time && (
                max_time < best_max_time || scheduler.gate_count < gate_count
            )) {
                best_dir = dir;
                best_map_list = dir ? stored_map_list : scheduler.map_list;
                best_max_time = max_time;
                gate_count = scheduler.gate_count;
                sum_min_route_len = scheduler.sum_min_route_len;
                sum_cnot_route_len = scheduler.sum_cnot_route_len;
                sum_rx_route_len = scheduler.sum_rx_route_len;
                max_route_len = scheduler.max_route_len;
                best_output = output.str();
                which_best = i;
            }
            stored_map_list = scheduler.map_list;
            dir = !dir;
            inst_list.reverse();
            scheduler.logical_gate_list = inst_list;
            device->clear();
            output.str("");
            // cerr << "DEBUG: turn " << i << " finished" << endl;
        }
        // -------- OUTPUT --------
        cout << filename 
//            << ", " << qcount
//            << ", " << parser.gate_count    // gate count of origin qasm program 
//            << ", " << parser.cnot_count
            << ", " << best_max_time 
            << ", " << gate_count
//            << ", " << which_best
//            << ", " << sum_min_route_len
//            << ", " << sum_rx_route_len
//            << ", " << sum_cnot_route_len
//            << ", " << max_route_len
            << endl;
	string ofn = outfilename ? string(outfilename) : string("output/") + filename;
        std::ofstream ofs(ofn, std::ios_base::out);
	ofs << "// working qubits:" << endl;
	ofs << "// ";
	for(size_t i = 0; i < best_map_list.size(); i++) {
           ofs << best_map_list[i] << ", ";
        }
	ofs << endl;
	ofs << "include \"qelib1.inc\";" << endl;
	ofs << "gate swap a, b {" << endl;
        ofs << "    cx a, b;" << endl;
        ofs << "    cx b, a;" << endl;
        ofs << "    cx a, b;" << endl;
	ofs << "}" << endl;
	ofs << "creg c[" << qcount << "];" << endl;
        ofs << "qreg q[" << m * n << "];" << endl;
        if(best_dir == false) {
            std::stack<std::string> lines;
            std::istringstream rawout(best_output);
            while(true) {
                char line[128];
                if(!rawout.getline(line, 128))
                    break;
                lines.emplace(line);
            }
            while(!lines.empty()) {
                ofs << lines.top() << endl;
                lines.pop();
            }
        } else {
            ofs << best_output;
        }
        std::ofstream mapfs(ofn + ".map", std::ios_base::out);
        for(size_t i = 0; i < best_map_list.size(); i++) {
            mapfs << i << " => " << best_map_list[i] << ";";
        }
    } catch (const std::exception &e) {
        cerr << "error: " << e.what() << endl;
        cerr << "\tat: " << parser.getCurrentLine() << endl;
        return -1;
    }
    delete device;
    return 0;
}
