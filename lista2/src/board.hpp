#pragma once
#include <bitset>
#include <cstdint>
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
    void set_metric_one(std::function<std::int64_t(move)> metric);
    void set_metric_two(std::function<std::int64_t(move)> metric);

    void set_piece(int x, int y, int player);
    int get_piece(int x, int y) const;
    void remove_piece(int x, int y);
    void move_piece(int x1, int y1, int x2, int y2);
    void move_piece(move move) { move_piece(move.x1, move.y1, move.x2, move.y2); }
    void undo_move(int x1, int y1, int x2, int y2);
    void undo_move(move move) { undo_move(move.x1, move.y1, move.x2, move.y2); }

    std::vector<move> get_legal_moves(int player) const;
    int get_winner() const;
    std::int64_t get_heuristic_one() const;
    std::int64_t get_heuristic_two() const;

    std::uint64_t hash_position() const;

    void copy_position(std::uint8_t out[BOARD_SIZE][BOARD_SIZE]);
    bool compare_position(std::uint8_t in[BOARD_SIZE][BOARD_SIZE]);

private:
    std::uint8_t _pieces[BOARD_SIZE][BOARD_SIZE];
    std::function<std::int64_t(move)> _metric_one;
    std::function<std::int64_t(move)> _metric_two;
    std::vector<std::pair<int, int>> _player_one_pieces;
    std::vector<std::pair<int, int>> _player_two_pieces;
    int _player_one_ok_pieces;
    int _player_two_ok_pieces;

    std::int64_t _heuristic_one;
    std::int64_t _heuristic_two;

    void rec_add_jump_moves(std::vector<move>& out,
        std::bitset<BOARD_SIZE * BOARD_SIZE>& visited,
        std::pair<int, int> from, std::pair<int, int> pos) const;
};
