#include <iostream>

#include "board.hpp"

int main() {

    board brd;
    brd.set_piece(15, 15, 1);
    auto test = brd.get_legal_moves(1);

    return 0;
}
