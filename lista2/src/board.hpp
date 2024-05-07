#pragma once
#include <bitset>
#include <functional>
#include <utility>
#include <vector>

#define BOARD_SIZE              16

class board {
public:
    struct move {
        int x1, y1;
        int x2, y2;
    };

    board();
    void set_metric(std::function<float(move)> metric);

    void set_piece(int x, int y, int player);
    void remove_piece(int x, int y);
    void move_piece(int x1, int y1, int x2, int y2);

    std::vector<move> get_legal_moves(int player) const;
    int get_winner() const;
    int get_heuristic() const;

private:
    int _pieces[BOARD_SIZE][BOARD_SIZE];
    std::function<float(move)> _metric;
    std::vector<std::pair<int, int>> _player_one_pieces;
    std::vector<std::pair<int, int>> _player_two_pieces;
    int _player_one_ok_pieces;
    int _player_two_ok_pieces;

    int _heuristic;

    void rec_add_jump_moves(std::vector<move>& out,
        std::bitset<BOARD_SIZE * BOARD_SIZE>& visited,
        std::pair<int, int> from, std::pair<int, int> pos) const;
};
