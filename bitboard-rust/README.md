``cargo run --release`

Funciona bien hasta perft 6 pero falla el perft 7.

Para comprobar:

https://gist.github.com/peterellisjones/8c46c28141c162d1d8a0f0badbc9cff9

```json
[
   {
      "depth":3,
      "nodes":62379,
      "fen":"rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8"
   },
   {
      "depth":4,
      "nodes":1274206,
      "fen":"r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1"
   },
   {
      "depth":4,
      "nodes":1720476,
      "fen":"r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1"
      // line 709 de board.rs, se captura pieza y no funciona el código
   }
]
```