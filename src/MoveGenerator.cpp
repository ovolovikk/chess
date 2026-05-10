#include "MoveGenerator.hpp"
#include <cmath>

bool MoveGenerator::canMoveTo(const Board& board, Position to, Color ownColor) const {
    if (!board.isInside(to)) return false;
    Piece target = board.getPiece(to);
    return target.type == PieceType::None || target.color != ownColor;
}

bool MoveGenerator::isEnemyPiece(const Board& board, Position pos, Color ownColor) const {
    Piece target = board.getPiece(pos);
    return target.type != PieceType::None && target.color != ownColor;
}

std::vector<Move> MoveGenerator::generatePseudoLegalMoves(const GameState& state, Position from) const {
    std::vector<Move> moves;
    Piece piece = state.board.getPiece(from);

    if (piece.type == PieceType::None || piece.color != state.sideToMove)
        return moves;

    const std::vector<Position> bishopDirections = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
    const std::vector<Position> rookDirections = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    const std::vector<Position> queenDirections = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    switch (piece.type) {
    case PieceType::Pawn:
        generatePawnMoves(state, from, moves);
        break;
    case PieceType::Knight:
        generateKnightMoves(state, from, moves);
        break;
    case PieceType::Bishop:
        generateSlidingMoves(state, from, moves, bishopDirections);
        break;
    case PieceType::Rook:
        generateSlidingMoves(state, from, moves, rookDirections);
        break;
    case PieceType::Queen:
        generateSlidingMoves(state, from, moves, queenDirections);
        break;
    case PieceType::King:
        generateKingMoves(state, from, moves);
        break;
    default:
        break;
    }

    return moves;
}

std::vector<Move> MoveGenerator::generateLegalMoves(const GameState& state, Position from) const {
    std::vector<Move> pseudoMoves = generatePseudoLegalMoves(state, from);
    std::vector<Move> legalMoves;
    Piece movingPiece = state.board.getPiece(from);

    for (const Move& move : pseudoMoves) {
        GameState copy = state;
        copy.makeMoveWithoutValidation(move);
        
        if (!isKingInCheck(copy, movingPiece.color)) {
            legalMoves.push_back(move);
        }
    }

    return legalMoves;
}

void MoveGenerator::addPromotionMoves(std::vector<Move>& moves, Position from, Position to, bool isCapture) const {
    MoveType type = isCapture ? MoveType::PromotionCapture : MoveType::Promotion;
    moves.push_back({from, to, type, PieceType::Queen});
    moves.push_back({from, to, type, PieceType::Rook});
    moves.push_back({from, to, type, PieceType::Bishop});
    moves.push_back({from, to, type, PieceType::Knight});
}

void MoveGenerator::generatePawnMoves(const GameState& state, Position from, std::vector<Move>& moves) const {
    const Board& board = state.board;
    Piece pawn = board.getPiece(from);

    int direction = pawn.color == Color::White ? -1 : 1;
    int startRow = pawn.color == Color::White ? 6 : 1;
    int promotionRow = pawn.color == Color::White ? 0 : 7;

    Position oneForward{from.row + direction, from.col};

    if (board.isInside(oneForward) && board.isEmpty(oneForward)) {
        if (oneForward.row == promotionRow) {
            addPromotionMoves(moves, from, oneForward, false);
        } else {
            moves.push_back({from, oneForward, MoveType::Normal});
        }
        
        Position twoForward{from.row + 2 * direction, from.col};
        if (from.row == startRow && board.isEmpty(twoForward)) {
            moves.push_back({from, twoForward, MoveType::DoublePawnPush});
        }
    }

    for (int dc : {-1, 1}) {
        Position capture{from.row + direction, from.col + dc};
        if (!board.isInside(capture)) continue;

        if (isEnemyPiece(board, capture, pawn.color)) {
            if (capture.row == promotionRow) {
                addPromotionMoves(moves, from, capture, true);
            } else {
                moves.push_back({from, capture, MoveType::Capture});
            }
        }
        else if (state.enPassantTarget.has_value() && capture == state.enPassantTarget.value()) {
            moves.push_back({from, capture, MoveType::EnPassant});
        }
    }
}

void MoveGenerator::generateKnightMoves(const GameState& state, Position from, std::vector<Move>& moves) const {
    static const std::vector<Position> offsets = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2},
        {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };

    const Board& board = state.board;
    Piece knight = board.getPiece(from);

    for (const Position& offset : offsets) {
        Position to{from.row + offset.row, from.col + offset.col};
        if (canMoveTo(board, to, knight.color)) {
            Piece target = board.getPiece(to);
            MoveType type = target.type == PieceType::None ? MoveType::Normal : MoveType::Capture;
            moves.push_back({from, to, type});
        }
    }
}

void MoveGenerator::generateSlidingMoves(const GameState& state, Position from, std::vector<Move>& moves, const std::vector<Position>& directions) const {
    const Board& board = state.board;
    Piece piece = board.getPiece(from);

    for (const Position& dir : directions) {
        Position to{from.row + dir.row, from.col + dir.col};

        while (board.isInside(to)) {
            Piece target = board.getPiece(to);

            if (target.color == piece.color) break;

            MoveType type = target.type == PieceType::None ? MoveType::Normal : MoveType::Capture;
            moves.push_back({from, to, type});

            if (target.type != PieceType::None) break;

            to.row += dir.row;
            to.col += dir.col;
        }
    }
}

void MoveGenerator::generateKingMoves(const GameState& state, Position from, std::vector<Move>& moves) const {
    const std::vector<Position> directions = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };
    
    const Board& board = state.board;
    Piece king = board.getPiece(from);

    for (const Position& dir : directions) {
        Position to{from.row + dir.row, from.col + dir.col};
        if (canMoveTo(board, to, king.color)) {
            Piece target = board.getPiece(to);
            MoveType type = target.type == PieceType::None ? MoveType::Normal : MoveType::Capture;
            moves.push_back({from, to, type});
        }
    }

    // simplistic Castling implementation - assuming King has not moved
    if (king.color == Color::White) {
        if (state.castlingRights.whiteKingSide && board.isEmpty({7, 5}) && board.isEmpty({7, 6})) {
            if (!isSquareAttacked(state, {7, 4}, Color::Black) && !isSquareAttacked(state, {7, 5}, Color::Black) && !isSquareAttacked(state, {7, 6}, Color::Black)) {
                moves.push_back({from, {7, 6}, MoveType::Castling});
            }
        }
        if (state.castlingRights.whiteQueenSide && board.isEmpty({7, 1}) && board.isEmpty({7, 2}) && board.isEmpty({7, 3})) {
            if (!isSquareAttacked(state, {7, 4}, Color::Black) && !isSquareAttacked(state, {7, 3}, Color::Black) && !isSquareAttacked(state, {7, 2}, Color::Black)) {
                moves.push_back({from, {7, 2}, MoveType::Castling});
            }
        }
    } else {
        if (state.castlingRights.blackKingSide && board.isEmpty({0, 5}) && board.isEmpty({0, 6})) {
            if (!isSquareAttacked(state, {0, 4}, Color::White) && !isSquareAttacked(state, {0, 5}, Color::White) && !isSquareAttacked(state, {0, 6}, Color::White)) {
                moves.push_back({from, {0, 6}, MoveType::Castling});
            }
        }
        if (state.castlingRights.blackQueenSide && board.isEmpty({0, 1}) && board.isEmpty({0, 2}) && board.isEmpty({0, 3})) {
            if (!isSquareAttacked(state, {0, 4}, Color::White) && !isSquareAttacked(state, {0, 3}, Color::White) && !isSquareAttacked(state, {0, 2}, Color::White)) {
                moves.push_back({from, {0, 2}, MoveType::Castling});
            }
        }
    }
}

// Minimal implementation to check if a square is attacked
bool MoveGenerator::isSquareAttacked(const GameState& state, Position square, Color byColor) const {
    const Board& board = state.board;

    // Check pawn attacks
    int pawnDir = byColor == Color::White ? 1 : -1; 
    for (int dc : {-1, 1}) {
        Position p{square.row + pawnDir, square.col + dc};
        if (board.isInside(p)) {
            Piece pc = board.getPiece(p);
            if (pc.type == PieceType::Pawn && pc.color == byColor) return true;
        }
    }

    // Knights
    static const std::vector<Position> knightOffsets = {
        {-2, -1}, {-2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}, {2, -1}, {2, 1}
    };
    for (auto ofs : knightOffsets) {
        Position p{square.row + ofs.row, square.col + ofs.col};
        if (board.isInside(p)) {
            Piece pc = board.getPiece(p);
            if (pc.type == PieceType::Knight && pc.color == byColor) return true;
        }
    }

    // Sliding pieces (rays)
    const std::vector<Position> allDirs = {
        {-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}
    };
    for (auto dir : allDirs) {
        Position p{square.row + dir.row, square.col + dir.col};
        while (board.isInside(p)) {
            Piece pc = board.getPiece(p);
            if (pc.type != PieceType::None) {
                if (pc.color == byColor) {
                    bool diag = dir.row != 0 && dir.col != 0;
                    if (pc.type == PieceType::Queen ||
                        (diag && pc.type == PieceType::Bishop) ||
                        (!diag && pc.type == PieceType::Rook)) {
                        return true;
                    }
                }
                break;
            }
            p.row += dir.row; p.col += dir.col;
        }
    }

    // King attacks
    for (auto dir : allDirs) {
        Position p{square.row + dir.row, square.col + dir.col};
        if (board.isInside(p)) {
            Piece pc = board.getPiece(p);
            if (pc.type == PieceType::King && pc.color == byColor) return true;
        }
    }

    return false;
}

bool MoveGenerator::isKingInCheck(const GameState& state, Color kingColor) const {
    const Board& board = state.board;
    Position kingPos{-1, -1};
    
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            Piece pc = board.getPiece({r, c});
            if (pc.type == PieceType::King && pc.color == kingColor) {
                kingPos = {r, c};
                break;
            }
        }
        if (kingPos.row != -1) break;
    }

    if (kingPos.row == -1) return false; // Missing king

    return isSquareAttacked(state, kingPos, opposite(kingColor));
}