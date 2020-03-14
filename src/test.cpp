#include <iostream>
#include "QScheduler.h"
#include "ExGridDevice.h"

using std::cin, std::cout, std::cerr, std::string, std::endl;
using namespace CODAR;

int main() {
        loadCommutingTable();
        // tokyo
        int m = 4;
        int n = 5;
        #define QPAIR(Q1, Q2)   lines.emplace_back(Q1, Q2);
        std::vector<std::pair<int, int>> lines;
        QPAIR(0, 1);    QPAIR(0, 5);
        QPAIR(1, 0);    QPAIR(1, 2);    QPAIR(1, 6);    QPAIR(1, 7);
        QPAIR(2, 1);    QPAIR(2, 3);    QPAIR(2, 6);    QPAIR(2, 7);
        QPAIR(3, 2);    QPAIR(3, 4);    QPAIR(3, 8);    QPAIR(3, 9);
        QPAIR(4, 3);    QPAIR(4, 8);    QPAIR(4, 9);
        QPAIR(5, 0);    QPAIR(5, 6);    QPAIR(5, 10);   QPAIR(5, 11);
        QPAIR(6, 1);    QPAIR(6, 2);    QPAIR(6, 5);    QPAIR(6, 7);    QPAIR(6, 11);
        QPAIR(7, 1);    QPAIR(7, 2);    QPAIR(7, 6);    QPAIR(7, 8);    QPAIR(7, 12);   QPAIR(7, 13);
        QPAIR(8, 3);    QPAIR(8, 4);    QPAIR(8, 7);    QPAIR(8, 9);    QPAIR(8, 12);   QPAIR(8, 13);
        QPAIR(9, 3);    QPAIR(9, 4);    QPAIR(9, 8);    QPAIR(9, 14);
        QPAIR(10, 5);   QPAIR(10, 6);   QPAIR(10, 11);  QPAIR(10, 15);
        QPAIR(11, 5);   QPAIR(11, 6);   QPAIR(11, 10);  QPAIR(11, 12);  QPAIR(11, 16);  QPAIR(11, 17);
        QPAIR(12, 7);   QPAIR(12, 8);   QPAIR(12, 11);  QPAIR(12, 13);  QPAIR(12, 16);  QPAIR(12, 17);
        QPAIR(13, 7);   QPAIR(13, 8);   QPAIR(13, 12);  QPAIR(13, 14);  QPAIR(13, 18);  QPAIR(13, 19);
        QPAIR(14, 9);   QPAIR(14, 13);  QPAIR(14, 18);  QPAIR(14, 19);
        QPAIR(15, 10);  QPAIR(15, 16);
        QPAIR(16, 11);  QPAIR(16, 12);  QPAIR(16, 15);  QPAIR(16, 17);
        QPAIR(17, 11);  QPAIR(17, 12);  QPAIR(17, 16);  QPAIR(17, 18);
        QPAIR(18, 13);  QPAIR(18, 14);  QPAIR(18, 17);  QPAIR(18, 19);
        QPAIR(19, 14);  QPAIR(19, 18);
        BasicGridDevice *device = new ExGridDevice(4, 5, lines);
        assert(device->isNearBy(1, 3, 2, 2));

        int qcount = 3;
        QScheduler scheduler(device);
        scheduler.addLogicalQubit(0, 3);
        scheduler.addLogicalQubit(1, 1);
        scheduler.addLogicalQubit(2, 0);
        scheduler.addCNotGate("cx", 0, 1);
        scheduler.addCNotGate("cx", 0, 2);
        scheduler.start();

        return 0;
}