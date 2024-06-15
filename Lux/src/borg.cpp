#include <iostream>
#include <string>
#include <random>

// tool for calculating funky sort borg code

int main( int argc, char** argv ) {
/*   unsigned long long borg_code = 0;
    unsigned int key = 0;
    bool up_diag, down_diag, upper, lower, left, right;

    for( key = 0; key < 64; key ++ ) {
        right = key & 1;
        left = key & 2;
        lower = key & 4;
        upper = key & 8;
        down_diag = key & 16;
        up_diag = key & 32;

        if( right || upper ) borg_code |= ( 1ULL << key );
    } */

    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis(0, std::numeric_limits<uint64_t>::max());
    uint64_t borg_code = dis(gen);

    // Print borg code in hexadecimal
    std::cout << "0x" << std::hex << borg_code << std::endl;
    return 0;
}