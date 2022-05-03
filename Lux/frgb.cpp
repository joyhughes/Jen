#include "frgb.hpp"

int main() {
    frgb color( -1.5f, 0.5f, 2.5f );
    std::cout << "Initial color     " << color << "\n\n";
    color.clip( -1.0f, 2.0f );
    std::cout << "Clipped color     " << color << "\n\n";
    color.constrain();
    std::cout << "Constrained color " << color << "\n\n";

    return 0;
}
