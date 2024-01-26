#include "base.cpp"
#include "lookup.cpp"

#include <string>
#include <cstdint>

struct statusReport {
    uint64_t checkCount;
    uint64_t kingIndex;
    Squares checkMask;
    Squares kingBan;
    Squares pinHV;
    Squares pinD;
    Squares enemySeen;
    Squares selfOcc;
    Squares enemyOcc;
    Squares epPin;
};

template<GameState state>
statusReport check(const Pieces &self, const Pieces &enemy) {
    Squares selfOcc = self.occupied();
    Squares enemyOcc = enemy.occupied();
    Squares occ = selfOcc | enemyOcc; 

    Squares occNotKing = occ & ~self.k;

    uint64_t kingIndex = _tzcnt_u64(self.k);
    uint64_t checkCount = 0;

    Squares checkMask = 0;
    Squares kingBan = 0;
    Squares pinHV = 0;
    Squares pinD = 0;
    Squares epPin = 0;

    Squares enemySeen;

    for (Squares temp = enemy.r | enemy.q; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = rookMoves[pieceIndex];
        Squares rookSeen = rookSlide(atk, occ, pieceIndex);
        enemySeen |= rookSeen;

        Squares pin = PinBetween[kingIndex][pieceIndex];

        if (rookSeen & self.k) { // enemy rook can see self king, so no inbetween pieces
            checkMask |= pin;
            kingBan |= CheckBetween[kingIndex][pieceIndex];
            checkCount++;
        } else if (atk & self.k) { // there are pieces in the way
            Squares inbetween = (pin ^ _blsi_u64(temp)) & occNotKing;
            Squares onePieceRemoved = _blsr_u64(inbetween);
            if (!onePieceRemoved) { // i.e. the enemy rook/queen is blocked by a single piece, then this piece is pinned
                pinHV |= pin;
            } else if (!_blsr_u64(onePieceRemoved)) { // there are two pieces in the way, therefore the only way for a piece to be pinned is that the piece is an enpassant pawn
                if (state.ep && (pieceIndex/8 == kingIndex/8)) { 
                    // there is enpassant pawn for enemy on the same file as the king
                    epPin = inbetween & self.p; // i.e. mask the self pawn in the pinned row so that it cannot take enpassant (if pawn exists there)
                }
            }
        }
    }

    for (Squares temp = enemy.b | enemy.q; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = bishopMoves[pieceIndex];
        Squares bishopSeen = rookSlide(atk, occ, pieceIndex);
        enemySeen |= bishopSeen;

        Squares pin = PinBetween[kingIndex][pieceIndex];

        if (bishopSeen & self.k) { // enemy rook can see self king, so no inbetween pieces
            checkMask |= pin;
            kingBan |= CheckBetween[kingIndex][pieceIndex];
            checkCount++;
        } else if (atk & self.k) { // there are pieces in the way
            Squares inbetween = (pin ^ _blsi_u64(temp)) & occNotKing;
            Squares onePieceRemoved = _blsr_u64(inbetween);
            if (!onePieceRemoved) { // i.e. the enemy bishop/queen is blocked by a single piece, then this piece is pinned
                pinHV |= pin;
            }
        }
    }

    for (Squares temp = enemy.n; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = knightMoves[pieceIndex];
        enemySeen |= atk;

        if (atk & self.k) {
            checkMask |= _blsi_u64(temp); 
        } 
    }

    // pawns
    Squares atkL;
    Squares atkR;
    if constexpr (state.isWhite) {
        atkL = (enemy.p & notAfile) >> 7;
        atkR = (enemy.p & notHfile) >> 9;
        if (atkL & self.k) checkMask |= (self.k << 7) & enemy.p;
        if (atkR & self.k) checkMask |= (self.k << 9) & enemy.p;
    } else {
        atkL = (enemy.p & notAfile) << 7;
        atkR = (enemy.p & notHfile) << 9;
        if (atkL & self.k) checkMask |= (self.k >> 7) & enemy.p;
        if (atkR & self.k) checkMask |= (self.k >> 9) & enemy.p;
    }
    enemySeen |= atkL | atkR;
    enemySeen |= kingMoves[bitFromSquare(enemy.k)];

    return {checkCount, kingIndex, checkMask, kingBan, pinHV, pinD, enemySeen, selfOcc, enemyOcc, epPin};
}