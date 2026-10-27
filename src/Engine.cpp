#include "Engine.hpp"
#include "MoveGenerator.hpp"
#include <algorithm>
#include <array>
#include <random>

namespace {
constexpr int MATE = 1000000;      // score for being checkmated (offset by ply)
constexpr int INF  = 2000000;      // a value larger than any real evaluation
constexpr int MATE_THRESHOLD = MATE - 1000; // scores above this are forced mates

// A "null" move used as a sentinel (no TT/killer move available).
const Move NO_MOVE = { {-1, -1}, {-1, -1}, MoveType::Normal, PieceType::None };

// Material values in centipawns. The King's value is effectively infinite for
// search purposes, but the legal-move generator already forbids losing it, so
// it never contributes to material counting here.
int materialValue(PieceType type) {
    switch (type) {
    case PieceType::Pawn:   return 100;
    case PieceType::Knight: return 320;
    case PieceType::Bishop: return 330;
    case PieceType::Rook:   return 500;
    case PieceType::Queen:  return 900;
    default:                return 0;
    }
}

// Piece-square tables, written from White's point of view in THIS project's
// coordinate system: row 0 is the top of the board (Black's back rank, rank 8)
// and row 7 is the bottom (White's back rank, rank 1). Values are centipawn
// bonuses added to a White piece standing on that square; for Black pieces we
// mirror vertically (row -> 7 - row) and subtract. These nudge the engine to
// develop knights/bishops toward the centre, push pawns, and castle.
using Table = std::array<std::array<int, 8>, 8>;

const Table PAWN_PST = {{
    {  0,   0,   0,   0,   0,   0,   0,   0},
    { 50,  50,  50,  50,  50,  50,  50,  50},
    { 10,  10,  20,  30,  30,  20,  10,  10},
    {  5,   5,  10,  25,  25,  10,   5,   5},
    {  0,   0,   0,  20,  20,   0,   0,   0},
    {  5,  -5, -10,   0,   0, -10,  -5,   5},
    {  5,  10,  10, -20, -20,  10,  10,   5},
    {  0,   0,   0,   0,   0,   0,   0,   0},
}};

const Table KNIGHT_PST = {{
    {-50, -40, -30, -30, -30, -30, -40, -50},
    {-40, -20,   0,   0,   0,   0, -20, -40},
    {-30,   0,  10,  15,  15,  10,   0, -30},
    {-30,   5,  15,  20,  20,  15,   5, -30},
    {-30,   0,  15,  20,  20,  15,   0, -30},
    {-30,   5,  10,  15,  15,  10,   5, -30},
    {-40, -20,   0,   5,   5,   0, -20, -40},
    {-50, -40, -30, -30, -30, -30, -40, -50},
}};

const Table BISHOP_PST = {{
    {-20, -10, -10, -10, -10, -10, -10, -20},
    {-10,   0,   0,   0,   0,   0,   0, -10},
    {-10,   0,   5,  10,  10,   5,   0, -10},
    {-10,   5,   5,  10,  10,   5,   5, -10},
    {-10,   0,  10,  10,  10,  10,   0, -10},
    {-10,  10,  10,  10,  10,  10,  10, -10},
    {-10,   5,   0,   0,   0,   0,   5, -10},
    {-20, -10, -10, -10, -10, -10, -10, -20},
}};

const Table ROOK_PST = {{
    {  0,   0,   0,   0,   0,   0,   0,   0},
    {  5,  10,  10,  10,  10,  10,  10,   5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    { -5,   0,   0,   0,   0,   0,   0,  -5},
    {  0,   0,   0,   5,   5,   0,   0,   0},
}};

const Table QUEEN_PST = {{
    {-20, -10, -10,  -5,  -5, -10, -10, -20},
    {-10,   0,   0,   0,   0,   0,   0, -10},
    {-10,   0,   5,   5,   5,   5,   0, -10},
    { -5,   0,   5,   5,   5,   5,   0,  -5},
    {  0,   0,   5,   5,   5,   5,   0,  -5},
    {-10,   5,   5,   5,   5,   5,   0, -10},
    {-10,   0,   5,   0,   0,   0,   0, -10},
    {-20, -10, -10,  -5,  -5, -10, -10, -20},
}};

// King table for the middlegame: stay tucked on the back rank, castled.
const Table KING_PST = {{
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-30, -40, -40, -50, -50, -40, -40, -30},
    {-20, -30, -30, -40, -40, -30, -30, -20},
    {-10, -20, -20, -20, -20, -20, -20, -10},
    { 20,  20,   0,   0,   0,   0,  20,  20},
    { 20,  30,  10,   0,   0,  10,  30,  20},
}};

const Table& tableFor(PieceType type) {
    switch (type) {
    case PieceType::Pawn:   return PAWN_PST;
    case PieceType::Knight: return KNIGHT_PST;
    case PieceType::Bishop: return BISHOP_PST;
    case PieceType::Rook:   return ROOK_PST;
    case PieceType::Queen:  return QUEEN_PST;
    default:                return KING_PST;
    }
}

bool sameMove(const Move& a, const Move& b) {
    return a.from == b.from && a.to == b.to &&
           a.promotionPiece == b.promotionPiece;
}
} // namespace

Engine::Engine() {
    // Deterministic Zobrist keys (fixed seed -> reproducible behaviour).
    std::mt19937_64 rng(0xC0FFEEULL);
    for (auto& color : m_pieceKeys)
        for (auto& type : color)
            for (auto& sq : type) sq = rng();
    m_sideKey = rng();
    for (auto& k : m_castleKeys) k = rng();
    for (auto& k : m_epKeys) k = rng();

    // ~4M-entry transposition table (~160 MB). Comfortable on an 8 GB machine
    // and big enough to make the search noticeably deeper. The browser build
    // gets ~1M entries (~40 MB) to keep the page's memory footprint sane.
#ifdef __EMSCRIPTEN__
    size_t entries = 1u << 20;
#else
    size_t entries = 1u << 22;
#endif
    m_tt.assign(entries, TTEntry{});
    m_ttMask = entries - 1;
}

int Engine::pieceValue(PieceType type) const {
    return materialValue(type);
}

int Engine::pieceSquareBonus(Piece piece, Position pos) const {
    const Table& table = tableFor(piece.type);
    // White uses the table as-is; Black mirrors it vertically.
    int row = piece.color == Color::White ? pos.row : 7 - pos.row;
    return table[row][pos.col];
}

int Engine::evaluate(const GameState& state) const {
    int score = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = state.board.getPiece({r, c});
            if (p.type == PieceType::None) continue;

            int value = materialValue(p.type) + pieceSquareBonus(p, {r, c});
            score += (p.color == Color::White) ? value : -value;
        }
    }
    return score;
}

uint64_t Engine::hash(const GameState& state) const {
    uint64_t h = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece p = state.board.getPiece({r, c});
            if (p.type == PieceType::None) continue;
            int ci = (p.color == Color::White) ? 0 : 1;
            int ti = static_cast<int>(p.type) - 1; // Pawn(1)..King(6) -> 0..5
            h ^= m_pieceKeys[ci][ti][r * 8 + c];
        }
    }
    if (state.sideToMove == Color::Black) h ^= m_sideKey;
    if (state.castlingRights.whiteKingSide)  h ^= m_castleKeys[0];
    if (state.castlingRights.whiteQueenSide) h ^= m_castleKeys[1];
    if (state.castlingRights.blackKingSide)  h ^= m_castleKeys[2];
    if (state.castlingRights.blackQueenSide) h ^= m_castleKeys[3];
    if (state.enPassantTarget.has_value())   h ^= m_epKeys[state.enPassantTarget->col];
    return h;
}

bool Engine::timeUp() {
    // Check the clock only every so often; calling now() is not free.
    if ((++m_nodes & 2047) != 0) return m_stop;
    if (std::chrono::steady_clock::now() >= m_deadline) m_stop = true;
    return m_stop;
}

std::vector<Move> Engine::allLegalMoves(const GameState& state) const {
    MoveGenerator gen;
    std::vector<Move> moves;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (state.board.getPiece({r, c}).color == state.sideToMove) {
                auto pieceMoves = gen.generateLegalMoves(state, {r, c});
                moves.insert(moves.end(), pieceMoves.begin(), pieceMoves.end());
            }
        }
    }
    return moves;
}

void Engine::orderMoves(const GameState& state, std::vector<Move>& moves,
                        const Move& ttMove, int ply) const {
    auto score = [&](const Move& m) -> int {
        if (sameMove(m, ttMove)) return 2000000; // best move from a prior search

        Piece victim = state.board.getPiece(m.to);
        int s = 0;
        if (victim.type != PieceType::None || m.type == MoveType::EnPassant) {
            // MVV-LVA: prefer winning big material with a small piece.
            Piece attacker = state.board.getPiece(m.from);
            int victimVal = (m.type == MoveType::EnPassant)
                                ? materialValue(PieceType::Pawn)
                                : materialValue(victim.type);
            s = 1000000 + 10 * victimVal - materialValue(attacker.type);
        } else if (ply < MAX_PLY &&
                   (sameMove(m, m_killers[ply][0]) || sameMove(m, m_killers[ply][1]))) {
            s = 900000; // quiet move that caused a cutoff before
        }
        if (m.promotionPiece != PieceType::None) s += materialValue(m.promotionPiece);
        return s;
    };
    std::sort(moves.begin(), moves.end(),
              [&](const Move& a, const Move& b) { return score(a) > score(b); });
}

int Engine::quiescence(const GameState& state, int alpha, int beta) {
    if (timeUp()) return 0;

    // Evaluate from the side-to-move's perspective.
    int sign = (state.sideToMove == Color::White) ? 1 : -1;
    int standPat = sign * evaluate(state);

    if (standPat >= beta) return beta;
    if (standPat > alpha) alpha = standPat;

    // Only search "noisy" moves (captures / promotions) to settle the position.
    std::vector<Move> moves = allLegalMoves(state);
    orderMoves(state, moves, NO_MOVE, MAX_PLY); // MAX_PLY -> skip killer lookup

    for (const Move& m : moves) {
        bool noisy = m.type == MoveType::Capture ||
                     m.type == MoveType::PromotionCapture ||
                     m.type == MoveType::EnPassant ||
                     m.type == MoveType::Promotion;
        if (!noisy) continue;

        GameState next = state;
        next.makeMoveWithoutValidation(m);
        int sc = -quiescence(next, -beta, -alpha);
        if (m_stop) return 0;

        if (sc >= beta) return beta;
        if (sc > alpha) alpha = sc;
    }
    return alpha;
}

int Engine::negamax(const GameState& state, int depth, int alpha, int beta, int ply) {
    if (timeUp()) return 0;

    const int alphaOrig = alpha;
    const uint64_t key = hash(state);
    TTEntry& entry = m_tt[key & m_ttMask];

    Move ttMove = NO_MOVE;
    if (entry.key == key) {
        ttMove = entry.best;
        if (entry.depth >= depth) {
            if (entry.bound == Bound::Exact) return entry.score;
            if (entry.bound == Bound::Lower && entry.score > alpha) alpha = entry.score;
            else if (entry.bound == Bound::Upper && entry.score < beta) beta = entry.score;
            if (alpha >= beta) return entry.score;
        }
    }

    if (depth == 0) {
        return quiescence(state, alpha, beta);
    }

    std::vector<Move> moves = allLegalMoves(state);
    if (moves.empty()) {
        MoveGenerator gen;
        if (gen.isKingInCheck(state, state.sideToMove)) {
            return -(MATE - ply); // Checkmate; faster mates score higher.
        }
        return 0; // Stalemate.
    }

    orderMoves(state, moves, ttMove, ply);

    int best = -INF;
    Move bestMove = moves[0];
    for (const Move& m : moves) {
        GameState next = state;
        next.makeMoveWithoutValidation(m);
        int sc = -negamax(next, depth - 1, -beta, -alpha, ply + 1);
        if (m_stop) return 0;

        if (sc > best) {
            best = sc;
            bestMove = m;
        }
        if (best > alpha) alpha = best;
        if (alpha >= beta) {
            // Beta cutoff. Remember quiet moves as killers for this ply.
            bool isQuiet = state.board.getPiece(m.to).type == PieceType::None &&
                           m.type != MoveType::EnPassant;
            if (isQuiet && ply < MAX_PLY && !sameMove(m, m_killers[ply][0])) {
                m_killers[ply][1] = m_killers[ply][0];
                m_killers[ply][0] = m;
            }
            break;
        }
    }

    // Store the result in the transposition table.
    entry.key = key;
    entry.score = best;
    entry.depth = static_cast<int16_t>(depth);
    entry.best = bestMove;
    if (best <= alphaOrig)      entry.bound = Bound::Upper;
    else if (best >= beta)      entry.bound = Bound::Lower;
    else                        entry.bound = Bound::Exact;

    return best;
}

std::optional<Move> Engine::findBestMove(const GameState& state) {
    std::vector<Move> moves = allLegalMoves(state);
    if (moves.empty()) return std::nullopt;

    // Only one legal move? Play it instantly, no thinking needed.
    if (moves.size() == 1) {
        lastDepth = 0;
        return moves[0];
    }

    // An "easy move": once the best move is at least this far ahead of every
    // alternative (and we've looked at least this deep to rule out a trap), it
    // is obviously best, so stop deepening instead of using the full time limit.
    constexpr int EASY_MARGIN = 200; // best is 2+ pawns clear of every rival
    constexpr int EASY_DEPTH = 4;

    m_stop = false;
    m_nodes = 0;
    m_deadline = std::chrono::steady_clock::now() +
                 std::chrono::milliseconds(timeLimitMs);
    for (auto& k : m_killers) { k[0] = NO_MOVE; k[1] = NO_MOVE; }

    Move bestMove = moves[0];
    lastDepth = 0;

    // Iterative deepening: search depth 1, 2, 3, ... reusing the transposition
    // table and the previous best move for ordering, until time runs out.
    for (int depth = 1; depth <= maxDepth; ++depth) {
        int alpha = -INF, beta = INF;
        int iterScore = -INF;
        Move iterBest = bestMove;
        bool completed = true;

        orderMoves(state, moves, bestMove, 0); // search the prior best first

        for (const Move& m : moves) {
            GameState next = state;
            next.makeMoveWithoutValidation(m);
            int sc = -negamax(next, depth - 1, -beta, -alpha, 1);
            if (m_stop) { completed = false; break; }

            if (sc > iterScore) {
                iterScore = sc;
                iterBest = m;
            }
            if (sc > alpha) alpha = sc;
        }

        if (!completed) break; // discard this incomplete depth

        bestMove = iterBest;
        lastDepth = depth;

        // Found a forced mate -- no need to search deeper.
        if (iterScore > MATE_THRESHOLD || iterScore < -MATE_THRESHOLD) break;

        // Easy-move detection. We can't read an exact "second best" score from
        // alpha-beta (non-best moves only return bounds), so instead we verify
        // directly: is every other move worse than (best - EASY_MARGIN)? A
        // null-window scout answers that cheaply, and since the main search
        // just populated the transposition table for these positions, the
        // scout mostly hits the TT and returns almost instantly.
        if (depth >= EASY_DEPTH && iterScore < MATE_THRESHOLD) {
            int threshold = iterScore - EASY_MARGIN;
            bool clear = true;
            for (const Move& m : moves) {
                if (sameMove(m, bestMove)) continue;
                GameState next = state;
                next.makeMoveWithoutValidation(m);
                int sc = -negamax(next, depth - 1, -(threshold + 1), -threshold, 1);
                if (m_stop) { clear = false; break; }
                if (sc > threshold) { clear = false; break; } // a rival is close
            }
            if (clear) break; // best move is obviously best -- stop thinking
        }
    }

    return bestMove;
}
