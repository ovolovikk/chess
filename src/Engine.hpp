#pragma once
#include "GameState.hpp"
#include "Types.hpp"
#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

// A simple but solid chess engine.
//
// Search:  iterative deepening over a negamax + alpha-beta search, backed by a
//          transposition table (Zobrist-hashed), a quiescence search (so it
//          does not hang pieces on the search horizon), and move ordering
//          (TT move, MVV-LVA captures, killer moves). Iterative deepening means
//          it searches depth 1, then 2, then 3... reusing what it learned, and
//          stops when it either reaches `maxDepth` or runs out of `timeLimitMs`.
//
// Evaluation: material counting in centipawns (Pawn=100, Knight=320, Bishop=330,
//          Rook=500, Queen=900 -- i.e. the classic 1/3/3/5/9 scale) plus light
//          piece-square tables so the engine develops its pieces sensibly.
//          Scores are always measured from White's point of view: positive
//          means White is better, negative means Black is better.
class Engine {
public:
    Engine();

    // Per-move thinking budget in milliseconds. The engine deepens until this
    // elapses (or `maxDepth` is hit), so deeper searches happen automatically
    // when the position is simple and time allows.
    int timeLimitMs = 5000;

    // Hard ceiling on search depth (plies). A genuine fixed depth this high is
    // computationally impossible; it just caps iterative deepening, which the
    // time limit normally stops well before this.
    int maxDepth = 15;

    // Best move for the side to move in `state`, or nullopt if there are none
    // (checkmate / stalemate).
    std::optional<Move> findBestMove(const GameState& state);

    // Static evaluation in centipawns, from White's perspective.
    int evaluate(const GameState& state) const;

    // Every legal move for the side to move.
    std::vector<Move> allLegalMoves(const GameState& state) const;

    // Depth actually reached on the last findBestMove call (for diagnostics).
    int lastDepth = 0;

private:
    enum class Bound : uint8_t { None, Exact, Lower, Upper };

    struct TTEntry {
        uint64_t key = 0;
        int32_t score = 0;
        int16_t depth = -1;
        Bound bound = Bound::None;
        Move best{};
    };

    static constexpr int MAX_PLY = 64;

    int negamax(const GameState& state, int depth, int alpha, int beta, int ply);
    int quiescence(const GameState& state, int alpha, int beta);

    void orderMoves(const GameState& state, std::vector<Move>& moves,
                    const Move& ttMove, int ply) const;

    uint64_t hash(const GameState& state) const;
    bool timeUp();

    int pieceValue(PieceType type) const;
    int pieceSquareBonus(Piece piece, Position pos) const;

    // Zobrist keys.
    uint64_t m_pieceKeys[2][6][64];
    uint64_t m_sideKey;
    uint64_t m_castleKeys[4];
    uint64_t m_epKeys[8];

    // Transposition table (power-of-two sized for cheap index masking).
    std::vector<TTEntry> m_tt;
    uint64_t m_ttMask = 0;

    // Killer moves: two quiet moves that caused a cutoff at each ply.
    Move m_killers[MAX_PLY][2];

    // Search bookkeeping.
    std::chrono::steady_clock::time_point m_deadline;
    uint64_t m_nodes = 0;
    bool m_stop = false;
};
