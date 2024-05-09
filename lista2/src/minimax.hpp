#pragma once
#include "board.hpp"

#include <cstdint>

class minimax {
public:
    minimax(int which_player, int which_heuristic);

    void set_depth(int depth);
    void make_next_move(board& board);

private:
    std::int64_t get_heuristic(const board& board);
    board::move get_best_move(board& board);
    std::int64_t alphabeta_rec(board& board, int player, int depth_left,
        std::int64_t alpha, std::int64_t beta);

    int _player;
    int _heuristic;
    int _depth;
};
