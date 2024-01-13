#include "display.cpp"
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>

Squares reachableNotBlocked(Squares reachableIfNotBlocked, Squares occupied, uint64_t pieceIndex) {
    BitLoop(reachableIfNotBlocked & occupied) {
        reachableIfNotBlocked &= attacking[pieceIndex][bitFromSquare(temp)];
    }
    return reachableIfNotBlocked;
}

std::array<std::vector<int>,65> bitpositions = { 
std::vector<int>({2, 3, 4, 5, 6, 7, 16, 24, 32, 40, 48, 56}),
std::vector<int>({1, 2, 3, 4, 5, 6, 7, 8, 16, 24, 32, 40, 48, 56}),
std::vector<int>({0, 2, 3, 4, 5, 6, 7, 9, 17, 25, 33, 41, 49, 57}),
std::vector<int>({0, 1, 3, 4, 5, 6, 7, 10, 18, 26, 34, 42, 50, 58}),
std::vector<int>({0, 1, 2, 4, 5, 6, 7, 11, 19, 27, 35, 43, 51, 59}),
std::vector<int>({0, 1, 2, 3, 5, 6, 7, 12, 20, 28, 36, 44, 52, 60}),
std::vector<int>({0, 1, 2, 3, 4, 6, 7, 13, 21, 29, 37, 45, 53, 61}),
std::vector<int>({0, 1, 2, 3, 4, 5, 7, 14, 22, 30, 38, 46, 54, 62}),
std::vector<int>({0, 1, 2, 3, 4, 5, 6, 15, 23, 31, 39, 47, 55, 63}),
std::vector<int>({0, 9, 10, 11, 12, 13, 14, 15, 16, 24, 32, 40, 48, 56}),
std::vector<int>({1, 8, 10, 11, 12, 13, 14, 15, 17, 25, 33, 41, 49, 57}),
std::vector<int>({2, 8, 9, 11, 12, 13, 14, 15, 18, 26, 34, 42, 50, 58}),
std::vector<int>({3, 8, 9, 10, 12, 13, 14, 15, 19, 27, 35, 43, 51, 59}),
std::vector<int>({4, 8, 9, 10, 11, 13, 14, 15, 20, 28, 36, 44, 52, 60}),
std::vector<int>({5, 8, 9, 10, 11, 12, 14, 15, 21, 29, 37, 45, 53, 61}),
std::vector<int>({6, 8, 9, 10, 11, 12, 13, 15, 22, 30, 38, 46, 54, 62}),
std::vector<int>({7, 8, 9, 10, 11, 12, 13, 14, 23, 31, 39, 47, 55, 63}),
std::vector<int>({0, 8, 17, 18, 19, 20, 21, 22, 23, 24, 32, 40, 48, 56}),
std::vector<int>({1, 9, 16, 18, 19, 20, 21, 22, 23, 25, 33, 41, 49, 57}),
std::vector<int>({2, 10, 16, 17, 19, 20, 21, 22, 23, 26, 34, 42, 50, 58}),
std::vector<int>({3, 11, 16, 17, 18, 20, 21, 22, 23, 27, 35, 43, 51, 59}),
std::vector<int>({4, 12, 16, 17, 18, 19, 21, 22, 23, 28, 36, 44, 52, 60}),
std::vector<int>({5, 13, 16, 17, 18, 19, 20, 22, 23, 29, 37, 45, 53, 61}),
std::vector<int>({6, 14, 16, 17, 18, 19, 20, 21, 23, 30, 38, 46, 54, 62}),
std::vector<int>({7, 15, 16, 17, 18, 19, 20, 21, 22, 31, 39, 47, 55, 63}),
std::vector<int>({0, 8, 16, 25, 26, 27, 28, 29, 30, 31, 32, 40, 48, 56}),
std::vector<int>({1, 9, 17, 24, 26, 27, 28, 29, 30, 31, 33, 41, 49, 57}),
std::vector<int>({2, 10, 18, 24, 25, 27, 28, 29, 30, 31, 34, 42, 50, 58}),
std::vector<int>({3, 11, 19, 24, 25, 26, 28, 29, 30, 31, 35, 43, 51, 59}),
std::vector<int>({4, 12, 20, 24, 25, 26, 27, 29, 30, 31, 36, 44, 52, 60}),
std::vector<int>({5, 13, 21, 24, 25, 26, 27, 28, 30, 31, 37, 45, 53, 61}),
std::vector<int>({6, 14, 22, 24, 25, 26, 27, 28, 29, 31, 38, 46, 54, 62}),
std::vector<int>({7, 15, 23, 24, 25, 26, 27, 28, 29, 30, 39, 47, 55, 63}),
std::vector<int>({0, 8, 16, 24, 33, 34, 35, 36, 37, 38, 39, 40, 48, 56}),
std::vector<int>({1, 9, 17, 25, 32, 34, 35, 36, 37, 38, 39, 41, 49, 57}),
std::vector<int>({2, 10, 18, 26, 32, 33, 35, 36, 37, 38, 39, 42, 50, 58}),
std::vector<int>({3, 11, 19, 27, 32, 33, 34, 36, 37, 38, 39, 43, 51, 59}),
std::vector<int>({4, 12, 20, 28, 32, 33, 34, 35, 37, 38, 39, 44, 52, 60}),
std::vector<int>({5, 13, 21, 29, 32, 33, 34, 35, 36, 38, 39, 45, 53, 61}),
std::vector<int>({6, 14, 22, 30, 32, 33, 34, 35, 36, 37, 39, 46, 54, 62}),
std::vector<int>({7, 15, 23, 31, 32, 33, 34, 35, 36, 37, 38, 47, 55, 63}),
std::vector<int>({0, 8, 16, 24, 32, 41, 42, 43, 44, 45, 46, 47, 48, 56}),
std::vector<int>({1, 9, 17, 25, 33, 40, 42, 43, 44, 45, 46, 47, 49, 57}),
std::vector<int>({2, 10, 18, 26, 34, 40, 41, 43, 44, 45, 46, 47, 50, 58}),
std::vector<int>({3, 11, 19, 27, 35, 40, 41, 42, 44, 45, 46, 47, 51, 59}),
std::vector<int>({4, 12, 20, 28, 36, 40, 41, 42, 43, 45, 46, 47, 52, 60}),
std::vector<int>({5, 13, 21, 29, 37, 40, 41, 42, 43, 44, 46, 47, 53, 61}),
std::vector<int>({6, 14, 22, 30, 38, 40, 41, 42, 43, 44, 45, 47, 54, 62}),
std::vector<int>({7, 15, 23, 31, 39, 40, 41, 42, 43, 44, 45, 46, 55, 63}),
std::vector<int>({0, 8, 16, 24, 32, 40, 49, 50, 51, 52, 53, 54, 55, 56}),
std::vector<int>({1, 9, 17, 25, 33, 41, 48, 50, 51, 52, 53, 54, 55, 57}),
std::vector<int>({2, 10, 18, 26, 34, 42, 48, 49, 51, 52, 53, 54, 55, 58}),
std::vector<int>({3, 11, 19, 27, 35, 43, 48, 49, 50, 52, 53, 54, 55, 59}),
std::vector<int>({4, 12, 20, 28, 36, 44, 48, 49, 50, 51, 53, 54, 55, 60}),
std::vector<int>({5, 13, 21, 29, 37, 45, 48, 49, 50, 51, 52, 54, 55, 61}),
std::vector<int>({6, 14, 22, 30, 38, 46, 48, 49, 50, 51, 52, 53, 55, 62}),
std::vector<int>({7, 15, 23, 31, 39, 47, 48, 49, 50, 51, 52, 53, 54, 63}),
std::vector<int>({0, 8, 16, 24, 32, 40, 48, 57, 58, 59, 60, 61, 62, 63}),
std::vector<int>({1, 9, 17, 25, 33, 41, 49, 56, 58, 59, 60, 61, 62, 63}),
std::vector<int>({2, 10, 18, 26, 34, 42, 50, 56, 57, 59, 60, 61, 62, 63}),
std::vector<int>({3, 11, 19, 27, 35, 43, 51, 56, 57, 58, 60, 61, 62, 63}),
std::vector<int>({4, 12, 20, 28, 36, 44, 52, 56, 57, 58, 59, 61, 62, 63}),
std::vector<int>({5, 13, 21, 29, 37, 45, 53, 56, 57, 58, 59, 60, 62, 63}),
std::vector<int>({6, 14, 22, 30, 38, 46, 54, 56, 57, 58, 59, 60, 61, 63}),
std::vector<int>({7, 15, 23, 31, 39, 47, 55, 56, 57, 58, 59, 60, 61, 62})
};

uint64_t pext_inverse(uint64_t values, uint64_t position) {
    uint64_t res = 0;

    for (auto id : bitpositions[position]) {
        res |= (values & 1ULL) << id;
        values >>=1;
    }

    return res;
}

int main() {

    uint64_t pos = 0b100101101110;

    display_int64(pext_inverse(pos, 0));

    display_int64(_pext_u64(pext_inverse(pos, 0), rookMoves[0] & ~kingMoves[0]));
    std::cout << std::hex << 1021840533 << std::endl;
    std::string filename = "lookup";

    for (int i=0; i<64; i++) {
        std::ofstream file(filename + std::to_string(i), std::ios::binary);
        if (!file) throw std::runtime_error("File IO error");
        for (int position = 0; position<(1<<bitpositions[i].size()); position++) {
            auto res = reachableNotBlocked(rookMoves[i], pext_inverse(position, i), i);
            file.write(reinterpret_cast<const char*>(&res), 8);
        }
        file.close();
    }

    return 0;
}