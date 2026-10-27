# Architecture

This is the internals doc for the codebase: how the pieces fit together and
why. For build/run instructions, controls, and the engine's search/eval
details, see [`README.md`](README.md) — this file focuses on structure so a
new session (human or AI) can find the right place to make a change.

## Shape of the app

A single-threaded SDL2 desktop app. One `while` loop in
[`main.cpp`](src/main.cpp) does: poll input → mutate `GameState` → render →
(if it's the AI's turn) let `Engine` search and apply its move → repeat.
There is no threading, no persistence, no networking — everything lives in
memory for one game and resets on checkmate/stalemate.

```
main.cpp
  ├── InputHandler      → polls SDL events into quit/click/key-toggle flags
  ├── GameState         → the single source of truth for "what is happening"
  │     ├── Board       → 8x8 piece array, no rules
  │     └── MoveGenerator (used internally to validate/apply moves and detect
  │                        checkmate/stalemate)
  ├── Engine            → search + evaluation; reads a GameState, never owns one
  └── Renderer + TextureManager → draws the current GameState, no game logic
```

Dependency direction is one-way: `Board` knows nothing above it;
`MoveGenerator` depends on `Board`+`GameState`; `Engine` depends on
`GameState`+`MoveGenerator`; `Renderer`/`TextureManager` depend on `Board` only
for drawing. Rendering code never mutates game state, and `Board` never
validates a move — that split is the main thing to preserve when adding
features.

## Modules

| File | Responsibility |
|------|----------------|
| [`Types.hpp`](src/Types.hpp) | Plain value types shared everywhere: `Color`, `PieceType`, `Piece`, `Position`, `Move`/`MoveType`, `CastlingRights`. No logic beyond `operator==` and `opposite(Color)`. |
| [`Board`](src/Board.hpp) | An 8×8 array of `Piece`. Get/set/move a piece, bounds/emptiness checks, starting position. Knows nothing about turns, check, or legality. |
| [`GameState`](src/GameState.hpp) | The full mutable game: board + side to move + castling rights + en-passant target + move clocks + UI selection + status. Owns move *application* (`makeMoveWithoutValidation`, the special-move handlers, castling-rights/en-passant bookkeeping) and turn/selection UI state (`selectSquare`, `tryMove`). |
| [`MoveGenerator`](src/MoveGenerator.hpp) | Stateless (no fields) move rules: pseudo-legal generation per piece type, `isSquareAttacked`, `isKingInCheck`, and legality filtering. |
| [`Engine`](src/Engine.hpp) | The AI: negamax/alpha-beta search with a transposition table, quiescence search, move ordering, and static evaluation. Pure function of a `GameState` snapshot — see [README § The engine](README.md#the-engine) for the algorithm. |
| [`InputHandler`](src/InputHandler.hpp) | Polls SDL events into three flags/values: quit requested, last mouse click position, AI-toggle key edge. No game knowledge. |
| [`Renderer`](src/Renderer.hpp) | All SDL2 drawing: board texture, pieces, selection/legal-move highlights, the eval bar, text. Owns the window/renderer/font. |
| [`TextureManager`](src/TextureManager.hpp) | Loads and caches one `SDL_Texture*` per (color, piece type). |
| [`main.cpp`](src/main.cpp) | Wires everything together and runs the frame loop. |

## Key conventions

- **Coordinates.** `Position{row, col}`, both `0..7`. `row 0` is the **top**
  (rank 8, Black's back rank); `row 7` is the **bottom** (rank 1, White's).
  `col 0` is the a-file. White pawns move toward *decreasing* row. The board
  is never flipped, even if the AI plays White.
- **Legality is enforced by copy-and-check, not by rule exclusion.**
  `MoveGenerator::generatePseudoLegalMoves` produces moves that follow piece
  movement rules but may leave the king in check;
  `generateLegalMoves` filters that list by copying the whole `GameState`,
  applying each move with `makeMoveWithoutValidation`, and discarding it if
  the mover's king is then in check (`Board`/`GameState` are small structs of
  fixed-size arrays, so copying is cheap — this is deliberately simple over
  clever). **Anything that reads "is this legal" must go through
  `MoveGenerator`, never re-implement a rule check inline.**
- **Two ways to apply a move, on purpose.** `makeMoveWithoutValidation` just
  applies board mechanics (used internally by legality checking and by the
  search, which needs to try moves that haven't been proven legal yet in that
  branch). `makeMove` is what the UI and the AI call once a move is already
  known to be legal — it also advances `fullmoveNumber`. Don't call
  `makeMove` on an unvalidated move, and don't skip it (i.e. call
  `makeMoveWithoutValidation` directly) from UI/AI code.
- **`GameState::updateStatus()` must be called after every move that reaches
  the board for real** (both the human path in `tryMove` and the AI path in
  `main.cpp`) — it's what detects checkmate/stalemate. It is *not* called
  during search or legality probing, since those apply moves to throwaway
  copies.
- **The engine only ever reads a `GameState`.** It takes one by
  `const&`/copies it internally for search branches; it never reaches back
  into the live game object. This keeps "what actually happened" (owned by
  `main.cpp`'s `GameState game;`) unambiguous even while the engine is mid-search
  on its own copies.
- **Evaluation is always from White's perspective** (positive centipawns =
  good for White), independent of who is human/AI. UI-facing code (the eval
  bar) is what flips sign/perspective for display — see
  [README § The evaluation bar](README.md#the-evaluation-bar).
- **Rendering is read-only.** `Renderer`/`TextureManager` take `const`
  references or values to draw; there is no path from rendering code back
  into `GameState` or `Board`.

## Adding a feature — quick guide

- **New piece behavior or special move** (e.g. a variant rule): add it to
  `MoveGenerator` (generation) and `GameState` (application in
  `makeMoveWithoutValidation` + any rights/state bookkeeping), following the
  existing `MoveType` cases. Don't special-case it in `main.cpp`.
- **New evaluation term** (e.g. king safety, mobility): add it inside
  `Engine::evaluate`, keep it in centipawns, keep the White-positive
  convention, and remember Black terms are mirrored/subtracted like the
  existing piece-square tables.
- **Draw rules (50-move, repetition, insufficient material):** these belong in
  `GameState::updateStatus()` (detection) using the clocks/state it already
  tracks — see *Known limitations* in the README, this is the main gap today.
- **Move history / undo / FEN:** would live on `GameState` (it already has
  the clocks and rights FEN needs); avoid duplicating that data elsewhere.
- **New UI element** (e.g. promotion-choice dialog, move list panel): add
  drawing to `Renderer`, add any new input handling to `InputHandler`, and
  wire the decision into `GameState`/`main.cpp` — keep `Renderer` itself free
  of game-rule logic.
- **Threading the AI search:** the boundary is `main.cpp`'s call to
  `engine.findBestMove(game)` — everything `Engine` needs is copied out of
  `GameState` up front (see `negamax`'s `GameState next = state;` pattern), so
  the search itself doesn't touch shared mutable state; moving it off the
  main thread is mostly a `main.cpp` concern (don't mutate `game` until the
  result comes back).
