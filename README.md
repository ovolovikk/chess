# Chess

A desktop chess game written in C++20 with SDL2, featuring a built-in chess
engine (AI opponent) and a live evaluation bar.

> This README documents the project as it currently stands so future sessions
> (human or AI) can get up to speed quickly. Keep it updated when behaviour
> changes.

---

## Quick start

### Dependencies (macOS, Homebrew)

```bash
brew install cmake sdl2 sdl2_image sdl2_ttf pkg-config
```

The build uses `pkg-config` to find SDL2, SDL2_image, and SDL2_ttf.

### Build

```bash
cd build
cmake ..        # defaults to a Release (optimized) build
make
```

### Run

The executable loads assets via the **relative path `../assets/...`**, so it
must be launched from inside the `build/` directory:

```bash
cd build
./chess
```

---

## Controls

| Input            | Action                                                            |
|------------------|-------------------------------------------------------------------|
| Left click       | Select one of your pieces, then click a highlighted square to move |
| **E**            | Toggle the AI opponent on/off                                      |
| Close window     | Quit                                                              |

- **Legal-move hints:** selecting a piece shows yellow dots for quiet moves and
  red squares for captures.
- **Pawn promotion** currently auto-promotes to a **Queen** (the UI does not yet
  ask which piece — see *Known limitations*).
- **Game over:** a checkmate/stalemate dialog appears, after which the game
  **auto-restarts**. The AI on/off state is preserved across restarts.

### Sides and the AI

- You ("blue") play **White**, at the bottom of the board, and move first.
- The AI plays **Black**. It is **off by default**; press **E** to enable it.
- While the AI is enabled it moves automatically whenever it is Black's turn,
  and your clicks on Black's turn are ignored.
- To change which side the AI plays, edit `HUMAN_COLOR` / `AI_COLOR` in
  [`src/main.cpp`](src/main.cpp).

The board is **not** flipped for the player; White is always rendered at the
bottom (rows 6–7) and Black at the top (rows 0–1).

---

## The evaluation bar

A vertical bar in the right-hand panel shows who is winning, updated every frame
from the engine's static evaluation:

- **Blue** fills from the **bottom** = the player (White) is ahead.
- **White** fills from the **top** = the opponent (Black) is ahead.
- A gray midline marks an even position.
- A numeric score (in pawns, e.g. `+1.5`) is shown from the player's
  perspective, on the leading side's end of the bar. Positive = good for you.
- "AI: ON/OFF" text shows the current AI state.

The centipawn score is mapped to the bar fill with a logistic curve
(`1 / (1 + e^(-cp/400))`) so small material edges are visible while large ones
saturate instead of overflowing.

The numeric label needs a system font; the renderer tries a few common macOS
font paths and silently skips the text if none load (the bar itself still
renders).

---

## Architecture

The code is a straightforward layered design under [`src/`](src/):

| File | Responsibility |
|------|----------------|
| [`Types.hpp`](src/Types.hpp) | Core value types: `Color`, `PieceType`, `Piece`, `Position`, `Move`, `MoveType`, `CastlingRights`, and the `opposite(Color)` helper. |
| [`Board.hpp`](src/Board.hpp) / [`.cpp`](src/Board.cpp) | An 8×8 `std::array` of `Piece`. Get/set/move pieces, bounds checks, and the starting position. **Row 0 is the top (Black back rank), row 7 is the bottom (White back rank).** |
| [`GameState.hpp`](src/GameState.hpp) / [`.cpp`](src/GameState.cpp) | The full game state: board, side to move, castling rights, en-passant target, move clocks, selection, and status. Applies moves (`makeMoveWithoutValidation`, `makeMove`) including castling/en-passant/promotion, and computes checkmate/stalemate (`updateStatus`). |
| [`MoveGenerator.hpp`](src/MoveGenerator.hpp) / [`.cpp`](src/MoveGenerator.cpp) | Pseudo-legal and fully-legal move generation per square, attack detection (`isSquareAttacked`), and check detection (`isKingInCheck`). Legality is enforced by playing the move on a copy and checking the king is not left in check. |
| [`Engine.hpp`](src/Engine.hpp) / [`.cpp`](src/Engine.cpp) | The chess AI (see below). |
| [`Renderer.hpp`](src/Renderer.hpp) / [`.cpp`](src/Renderer.cpp) | All SDL2 drawing: board texture, pieces, selection/legal-move highlights, and the evaluation bar. Owns the window, SDL renderer, and an optional TTF font. |
| [`TextureManager.hpp`](src/TextureManager.hpp) / [`.cpp`](src/TextureManager.cpp) | Loads and caches piece textures, keyed by color+type. |
| [`InputHandler.hpp`](src/InputHandler.hpp) / [`.cpp`](src/InputHandler.cpp) | Polls SDL events: quit, left-click position, and the `E` AI-toggle key. |
| [`main.cpp`](src/main.cpp) | Initializes SDL/IMG/TTF, loads assets, and runs the main loop (input → render → AI move → game-over handling). |

### Coordinate system

`Position{row, col}` with `row`/`col` in `0..7`. `row 0` is the **top** of the
board (rank 8, Black's side); `row 7` is the **bottom** (rank 1, White's side).
`col 0` is the left (a-file). White pawns move toward decreasing `row`.

---

## The engine

Implemented in [`Engine.cpp`](src/Engine.cpp).

### Search

- **Iterative deepening:** searches depth 1, then 2, then 3, … reusing what it
  learned each pass, until it hits the time limit or the depth ceiling. This is
  why it "goes as deep as it can" rather than running a fixed depth — and it
  guarantees a usable move is always ready.
- **Negamax with alpha-beta pruning.**
- **Transposition table** (~4M entries, Zobrist-hashed) caches previously
  searched positions, hugely reducing re-search of transpositions and feeding
  the best-known move into move ordering.
- **Quiescence search** at the leaves (searches only captures/promotions) so the
  engine doesn't stop the search mid-trade and hang or miss material.
- **Move ordering:** TT best move first, then MVV-LVA captures (Most Valuable
  Victim − Least Valuable Attacker), then killer moves (quiet moves that caused
  a cutoff at the same ply). Good ordering is what makes alpha-beta prune hard.
- **Mate scoring** is offset by ply so the engine prefers faster mates and
  delays getting mated; it stops deepening once a forced mate is found.
- **Easy-move early exit:** after each completed depth (≥ `EASY_DEPTH`, default
  4), it checks whether the best move is at least `EASY_MARGIN` centipawns
  (default 200, ≈2 pawns) clear of *every* alternative, using cheap null-window
  "scout" searches (which mostly hit the warm transposition table). If so it
  plays immediately instead of using the full time budget — e.g. snapping up a
  hung queen/rook/minor takes tens of milliseconds rather than seconds. Winning
  only a pawn does *not* trigger it, so genuinely close decisions still get the
  full budget. (Note: alpha-beta can't report an exact "second-best" score —
  non-best moves only return bounds — which is why the scout verification is
  needed rather than just comparing root scores.)

Two knobs control strength/speed (in [`Engine.hpp`](src/Engine.hpp)):

- `timeLimitMs` (default **2000**) — per-move thinking budget. The engine
  deepens until this elapses. Bigger = stronger and slower.
- `maxDepth` (default **15**) — a hard ceiling on iterative deepening. A genuine
  *fixed* depth this high is computationally impossible; the time limit normally
  stops the search well before reaching it. In simple/endgame positions it may
  reach the ceiling quickly; from the opening, ~2 s reaches roughly depth 7.

`Engine::lastDepth` reports the depth actually completed on the last search.

The engine plays each move synchronously in the main loop *after* the frame is
presented, so your move is visible while it thinks. (This blocks the UI for up
to `timeLimitMs` while it searches — see *Known limitations*.)

### Evaluation

`Engine::evaluate` returns a score in **centipawns from White's perspective**
(positive = White better). It sums:

1. **Material** — Pawn 100, Knight 320, Bishop 330, Rook 500, Queen 900
   (the classic 1/3/3/5/9 scale). The king is not materially counted; legal-move
   generation already forbids losing it.
2. **Piece-square tables** — small positional bonuses encouraging central
   knights/bishops, pawn advancement, rooks on open ranks, and a castled king.
   Tables are written from White's perspective in this project's coordinate
   system (row 0 = top); Black mirrors them vertically and the value is
   subtracted.

`Engine::allLegalMoves(state)` gathers every legal move for the side to move and
is reused by both the search and (potentially) the UI.

---

## Known limitations / TODO

These are intentional simplifications a future session might want to address:

- **Promotion UI:** always promotes to Queen; no piece-choice dialog.
- **Draw rules:** checkmate and stalemate are detected, but the 50-move rule,
  threefold repetition, and insufficient-material draws are **not**. `GameState`
  tracks `halfmoveClock`/`fullmoveNumber` but they aren't used for draw
  detection. The engine likewise has no repetition awareness.
- **UI blocks during AI search** for up to `timeLimitMs`. For a fully smooth UI,
  move the search to a worker thread and poll it from the main loop.
- **No board flipping** when playing as Black.
- **No move history / undo / FEN load/save.**
- **No opening book.** The engine plays principled opening moves from search
  (e.g. 1.e4), but a book would make the opening faster and more varied.
- **Transposition table is not ply-corrected for mate scores** (a minor search
  inaccuracy near forced mates) and is never explicitly cleared between moves
  (entries are overwritten by key, which is fine in practice).
- **Castling-rights edge case:** rights are cleared when the king or a rook
  *moves*, but not when a rook is *captured on its home square* — a known minor
  bug to verify/fix.
- **Font dependency:** the eval-bar number relies on finding a macOS system
  font; bundle a `.ttf` in `assets/` for portability.

---

## Project layout

```
chess/
├── CMakeLists.txt        # build config (C++20, SDL2/image/ttf, Release default)
├── README.md             # this file
├── assets/               # board.png + 12 piece PNGs (white/black × 6 types)
├── build/                # CMake build dir (gitignored); run ./chess from here
└── src/                  # all source (see Architecture table above)
```
