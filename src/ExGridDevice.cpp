#include "ExGridDevice.h"

namespace CODAR {

    ExGridDevice::ExGridDevice(int m, int n, std::vector<std::pair<int, int>> &lines)
            : BasicGridDevice(m, n) {
            is_simple_grid_device = false;
            qcount = m * n;
            dist_mat = new int[qcount * qcount];
            for(int i = 0; i < qcount * qcount; i++) {
                dist_mat[i] = __INT_MAX__ >> 2;
            }
            for(auto line : lines) {
                int i1 = line.first / n;
                int j1 = line.first % n;
                int i2 = line.second / n;
                int j2 = line.second % n;
                getQubit(i1, j1).nearbyQubits.emplace_back(i2, j2);
                dist_mat[line.first * qcount + line.second] = 1;
                dist_mat[line.second * qcount + line.first] = 1;
            }
            // floyd algorithm
            for(int k = 0; k < qcount; k++)
                for(int j = 0; j < qcount; j++)
                    for(int i = 0; i < qcount; i++) {
                        int dist = dist_mat[i * qcount + k] + dist_mat[k * qcount + j];
                        int &dist0 = dist_mat[i * qcount + j];
                        if(dist0 > dist) {
                            dist0 = dist;
                        }
                    }
        }

    
    ExGridDevice::~ExGridDevice() {
        delete[] dist_mat;
    }

    bool ExGridDevice::isNearBy(int i1, int j1, int i2, int j2) {
        int q1 = i1 * n + j1;
        int q2 = i2 * n + j2;
        return dist_mat[q1 * qcount + q2] == 1;
    }

    int ExGridDevice::getDistance(int i1, int j1, int i2, int j2) {
        int q1 = i1 * n + j1;
        int q2 = i2 * n + j2;
        return dist_mat[q1 * qcount + q2];
    }

} // CODAR