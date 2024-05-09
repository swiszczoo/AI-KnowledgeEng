#include "minimax.hpp"

#include <cassert>
#include <random>

minimax::minimax(int which_player, int which_heuristic)
    : _player(which_player)
    , _heuristic(which_heuristic)
    , _depth(4)
{
}

void minimax::set_depth(int depth)
{
    _depth = depth;
}

void minimax::make_next_move(board& board)
{
    auto move = get_best_move(board);
    board.move_piece(move);
}

std::int64_t minimax::get_heuristic(const board& board)
{
    assert(_heuristic == 1 || _heuristic == 2);
    assert(_player == 1 || _player == 2);

    std::int64_t sign = _player == 1 ? 1 : -1;
    return sign * (_heuristic == 1 ? board.get_heuristic_one() : board.get_heuristic_two());
}

static inline int next_player(int player)
{
    return player == 1 ? 2 : 1;
}

board::move minimax::get_best_move(board& board)
{
    auto moves = board.get_legal_moves(_player);
    std::int64_t best_move_score = LLONG_MIN;
    std::vector<board::move> best_moves;

    for (auto& move : moves) {
 
        board.move_piece(move);
        auto score = alphabeta_rec(board, _player, _depth - 1, LLONG_MIN, LLONG_MAX);

        if (best_move_score == LLONG_MIN || score > best_move_score) {
            best_moves.clear();
            best_moves.emplace_back(move);
            best_move_score = score;
        }
        else if (score == best_move_score) {
            best_moves.emplace_back(move);
        }

        board.undo_move(move);
    }

    static std::random_device rd;
    static std::mt19937_64 mt(rd());

    std::uniform_int_distribution<size_t> dist(0, best_moves.size() - 1);
    return best_moves[dist(mt)];
}

std::int64_t minimax::alphabeta_rec(board& board, int player, int depth_left,
    std::int64_t alpha, std::int64_t beta)
{
    if (depth_left <= 0 || board.get_winner()) {
        return get_heuristic(board);
    }

    auto moves = board.get_legal_moves(player);
    if (player != _player) {
        for (auto& move : moves) {
            board.move_piece(move);
            beta = std::min(beta, alphabeta_rec(board, next_player(player), depth_left - 1, alpha, beta));
            board.undo_move(move);
            if (alpha >= beta) {
                break;
            }
        }

        return beta;
    }
    else {
        for (auto& move : moves) {
            board.move_piece(move);
            alpha = std::max(alpha, alphabeta_rec(board, next_player(player), depth_left - 1, alpha, beta));
            board.undo_move(move);
            if (alpha >= beta) {
                break;
            }
        }

        return alpha;
    }
}
