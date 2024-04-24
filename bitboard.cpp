#include "base.cpp"
#include "lookup.cpp"

#include <string>
#include <cstdint>
#include <vector>

#include <iostream>

void pSq(uint64_t num) {
    for (int row=7; row>=0; row--) {
        for (int col=0; col<8; col++) {
            if (num >> (8*row + col) & 0b1) std::cout << "X";
            else std::cout << "-";           
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

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

#define BitLoop(reachable) for (uint64_t temp=reachable; temp; temp=_blsr_u64(temp))

template<bool isWhite, bool ep>
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

    Squares enemySeen = 0;

    for (Squares temp = enemy.r | enemy.q; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = rookMoves[pieceIndex];
        Squares rookSeen = slide(atk, occ, pieceIndex);
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
                if (ep && (pieceIndex/8 == kingIndex/8)) { 
                    // there is enpassant pawn for enemy on the same file as the king
                    epPin = inbetween & self.p; // i.e. mask the self pawn in the pinned row so that it cannot take enpassant (if pawn exists there)
                }
            }
        }
    }

    for (Squares temp = enemy.b | enemy.q; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = bishopMoves[pieceIndex];
        Squares bishopSeen = slide(atk, occ, pieceIndex);
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
                pinD |= pin;
            }
        }
    }

    for (Squares temp = enemy.n; temp; temp = _blsr_u64(temp)) {
        uint64_t pieceIndex = _tzcnt_u64(temp);
        Squares atk = knightMoves[pieceIndex];
        enemySeen |= atk;

        if (atk & self.k) {
            checkCount++;
            checkMask |= _blsi_u64(temp); 
        } 
    }

    // pawns
    Squares atkL;
    Squares atkR;
    if constexpr (isWhite) {
        atkL = (enemy.p & notAfile) >> 9;
        atkR = (enemy.p & notHfile) >> 7;
        if (atkL & self.k) {checkMask |= (self.k << 9) & enemy.p; checkCount++;}
        if (atkR & self.k) {checkMask |= (self.k << 7) & enemy.p; checkCount++;}
    } else {
        atkL = (enemy.p & notAfile) << 7;
        atkR = (enemy.p & notHfile) << 9;
        if (atkL & self.k) {checkMask |= (self.k >> 7) & enemy.p; checkCount++;}
        if (atkR & self.k) {checkMask |= (self.k >> 9) & enemy.p; checkCount++;}
    }
    enemySeen |= atkL | atkR;
    enemySeen |= kingMoves[_tzcnt_u64(enemy.k)];

    return {checkCount, kingIndex, checkMask, kingBan, pinHV, pinD, enemySeen, selfOcc, enemyOcc, epPin};
}

template<bool isWhite, bool ep, bool wL, bool wR, bool bL, bool bR>
std::vector<Board> generateMoves(Board &board) {
    Pieces self, enemy;
    if constexpr (isWhite) {
        self = board.w; enemy = board.b;
    } else {
        self = board.b; enemy = board.w;
    }

    std::vector<Board> out;

    statusReport res = check<isWhite, ep>(self, enemy);
    Squares notEnemy = ~res.enemyOcc;
    Squares notSelf = ~res.selfOcc;
    Squares occ = res.selfOcc | res.enemyOcc;
    Squares notpin = ~(res.pinHV | res.pinD);

    // king moves
    {    
        Squares kingReachable = kingMoves[res.kingIndex] & ~res.kingBan & ~res.enemySeen & notSelf;
        BitLoop(kingReachable & notEnemy) { out.push_back(board.pieceMove<KING, isWhite>(_blsi_u64(temp) | self.k)); }
        BitLoop(kingReachable & res.enemyOcc) { out.push_back(board.pieceMoveCapture<KING, isWhite>(_blsi_u64(temp) | self.k)); }

        if (res.checkCount > 1) { // only king can move
            return out;
        }
    }

    if (res.checkCount == 0) res.checkMask = ~res.checkMask;
    Squares notselfCheckmask = notSelf & res.checkMask;

    // rook moves
    {
        // pinned
        for (Squares pinnedRooks = self.r & res.pinHV; pinnedRooks; pinnedRooks = _blsr_u64(pinnedRooks)) {
            Squares current = _blsi_u64(pinnedRooks);
            uint64_t pieceIndex = _tzcnt_u64(pinnedRooks);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<ROOK, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<ROOK, isWhite>(_blsi_u64(temp) | current)); }
        }
        // not pinned
        for (Squares unpinnedRooks = self.r & notpin; unpinnedRooks; unpinnedRooks = _blsr_u64(unpinnedRooks)) {
            Squares current = _blsi_u64(unpinnedRooks);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedRooks);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<ROOK, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<ROOK, isWhite>(_blsi_u64(temp) | current)); }
        }
    }

    // bishop moves
    {
        // pinned
        for (Squares pinnedBishops = self.b & res.pinD; pinnedBishops; pinnedBishops = _blsr_u64(pinnedBishops)) {
            Squares current = _blsi_u64(pinnedBishops);
            uint64_t pieceIndex = _tzcnt_u64(pinnedBishops);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinD;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<BISHOP, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<BISHOP, isWhite>(_blsi_u64(temp) | current)); }
        }
        // not pinned
        for (Squares unpinnedBishops = self.b & notpin; unpinnedBishops; unpinnedBishops = _blsr_u64(unpinnedBishops)) {
            Squares current = _blsi_u64(unpinnedBishops);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedBishops);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<BISHOP, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<BISHOP, isWhite>(_blsi_u64(temp) | current)); }
        }
    }

    // queen moves
    {
        for (Squares queensHV = self.q & res.pinHV; queensHV; queensHV = _blsr_u64(queensHV)) {
            Squares current = _blsi_u64(queensHV);
            uint64_t pieceIndex = _tzcnt_u64(queensHV);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
        }
        for (Squares queensD = self.q & res.pinD; queensD; queensD = _blsr_u64(queensD)) {
            Squares current = _blsi_u64(queensD);
            uint64_t pieceIndex = _tzcnt_u64(queensD);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinD;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
        }
        for (Squares unpinnedQueens = self.q & notpin; unpinnedQueens; unpinnedQueens = _blsr_u64(unpinnedQueens)) {
            Squares current = _blsi_u64(unpinnedQueens);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedQueens);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) | slide(bishopMoves[pieceIndex], occ, pieceIndex);
            atk &= notselfCheckmask;
            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<QUEEN, isWhite>(_blsi_u64(temp) | current)); }
        }
    }

    // knight moves
    {
        for (Squares unpinnedKnight = self.n & notpin; unpinnedKnight; unpinnedKnight = _blsr_u64(unpinnedKnight)) {
            Squares current = _blsi_u64(unpinnedKnight);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedKnight);
            Squares atk = knightMoves[pieceIndex] & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<KNIGHT, isWhite>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<KNIGHT, isWhite>(_blsi_u64(temp) | current)); }
        }
    }

    // pawns
    {
        Squares pawnAdvance;
        if constexpr (isWhite) pawnAdvance = self.p & ~wPawnLast & ~(occ >> 8); // tirar pabajo 1 fila
        else pawnAdvance = self.p & ~bPawnLast & ~(occ << 8);

        Squares pawnPush;
        if constexpr (isWhite) pawnPush = pawnAdvance & wPawnStart & ~(occ >> 16);
        else pawnPush = pawnAdvance & bPawnStart & ~(occ << 16);

        Squares pawnCaptureL;
        if constexpr (isWhite) pawnCaptureL = self.p & ~wPawnLast & ((res.enemyOcc & notHfile) >> 7);
        else pawnCaptureL = self.p & ~bPawnLast & ((res.enemyOcc & notAfile) << 7);

        Squares pawnCaptureR;
        if constexpr (isWhite) pawnCaptureR = self.p & ~wPawnLast & ((res.enemyOcc & notAfile) >> 9);
        else pawnCaptureR = self.p & ~bPawnLast & ((res.enemyOcc & notHfile) << 9);

        Squares pawnAdvancePromote;
        if constexpr (isWhite) pawnAdvancePromote = self.p & wPawnLast & ~(occ >> 8);
        else pawnAdvancePromote = self.p & bPawnLast & ~(occ << 8);

        Squares pawnCaptureLpromote;
        if constexpr (isWhite) pawnCaptureLpromote = self.p & wPawnLast & ((res.enemyOcc & notHfile) >> 7);
        else pawnCaptureLpromote = self.p & bPawnLast & ((res.enemyOcc & notAfile) << 7);

        Squares pawnCaptureRpromote;
        if constexpr (isWhite) pawnCaptureRpromote = self.p & wPawnLast & ((res.enemyOcc & notAfile) >> 9);
        else pawnCaptureRpromote = self.p & bPawnLast & ((res.enemyOcc & notHfile) << 9);

        // not pinned
        BitLoop(pawnAdvance & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 8;
            else final = current >> 8;
            if (final & notselfCheckmask) out.push_back(board.pieceMove<PAWN, isWhite>(current | final)); 
        }
        BitLoop(pawnPush & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 16;
            else final = current >> 16;
            if (final & notselfCheckmask) out.push_back(board.pawnPush<isWhite>(final, current | final)); 
        }
        BitLoop(pawnCaptureR & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 9;
            else final = current >> 9;
            if (final & notselfCheckmask) out.push_back(board.pieceMoveCapture<PAWN, isWhite>(current | final)); 
        }
        BitLoop(pawnCaptureL & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 7;
            else final = current >> 7;
            if (final & notselfCheckmask) out.push_back(board.pieceMoveCapture<PAWN, isWhite>(current | final)); 
        }
        // pinned
        BitLoop(pawnAdvance & res.pinHV) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 8;
            else final = current >> 8;            
            if (final & res.pinHV & notselfCheckmask) out.push_back(board.pieceMove<PAWN, isWhite>(current | final)); 
        }
        BitLoop(pawnPush & res.pinHV) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 16;
            else final = current >> 16;
            if (final & res.pinHV & notselfCheckmask) out.push_back(board.pawnPush<isWhite>(final, current | final)); 
        }
        BitLoop(pawnCaptureR & res.pinD) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 9;
            else final = current >> 9;
            if (final & res.pinD & notselfCheckmask) out.push_back(board.pieceMoveCapture<PAWN, isWhite>(current | final)); 
        }
        BitLoop(pawnCaptureL & res.pinD) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 7;
            else final = current >> 7;
            if (final & res.pinD & notselfCheckmask) out.push_back(board.pieceMoveCapture<PAWN, isWhite>(current | final)); 
        }
        // not pinned promotion
        BitLoop(pawnAdvancePromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 8;
            else final = current >> 8;
            if (final & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }

        }
        BitLoop(pawnCaptureLpromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 7;
            else final = current >> 7;
            if (final & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }
        }
        BitLoop(pawnCaptureRpromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 9;
            else final = current >> 9;
            if (final & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }
        }
        // pinned promotion
        BitLoop(pawnAdvancePromote & res.pinHV) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 8;
            else final = current >> 8;
            if (final & res.pinHV & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }
        }
        BitLoop(pawnCaptureLpromote & res.pinD) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 7;
            else final = current >> 7;
            if (final & res.pinHV & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }
        }
        BitLoop(pawnCaptureRpromote & res.pinD) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (isWhite) final = current << 9;
            else final = current >> 9;
            if (final & res.pinD & notselfCheckmask) {
                out.push_back(board.pawnPromote<QUEEN, isWhite>(current, final));
                out.push_back(board.pawnPromote<ROOK, isWhite>(current, final)); 
                out.push_back(board.pawnPromote<BISHOP, isWhite>(current, final));
                out.push_back(board.pawnPromote<KNIGHT, isWhite>(current, final));
            }
        }
    }

    // enpassant
    if constexpr (ep) {
        if (board.ep & ~res.pinD) { // enemy pawn is diagonal pinned (for self), cannot remove it

            Squares enemyPawnBehind;
            if constexpr (isWhite) enemyPawnBehind = board.ep << 8;
            else enemyPawnBehind = board.ep >> 8;

            // board.ep is the square of the enemy pawn that pushed
            if (board.ep & notAfile) { // can capture e.p. to the right

                // pawn must be to the left, not e.p. pinned and not HV pinned
                Squares pawnToTheLeft = (board.ep >> 1) & self.p & ~res.epPin & ~res.pinHV;
                // if the pawn is not diagonally pinned
                if ((pawnToTheLeft & ~res.pinD) && (enemyPawnBehind & notselfCheckmask)) out.push_back(board.pawnEP<isWhite>(board.ep, pawnToTheLeft | enemyPawnBehind));
                else if ((pawnToTheLeft & res.pinD) && (enemyPawnBehind & notselfCheckmask & res.pinD)) out.push_back(board.pawnEP<isWhite>(board.ep, pawnToTheLeft | enemyPawnBehind));
            } 
            if (board.ep & notHfile) { // can capture e.p. to the right

                // pawn must be to the left, not e.p. pinned and not HV pinned
                Squares pawnToTheRight = (board.ep << 1) & self.p & ~res.epPin & ~res.pinHV;
                // if the pawn is not diagonally pinned
                if ((pawnToTheRight & ~res.pinD) && (enemyPawnBehind & notselfCheckmask)) out.push_back(board.pawnEP<isWhite>(board.ep, pawnToTheRight | enemyPawnBehind));
                else if ((pawnToTheRight & res.pinD) && (enemyPawnBehind & notselfCheckmask & res.pinD)) out.push_back(board.pawnEP<isWhite>(board.ep, pawnToTheRight | enemyPawnBehind));
            }
        }
    }

    // castles
    if constexpr (isWhite) {
        if constexpr (wL) {
            if (res.checkCount == 0 && !((res.enemySeen | occ) & wLCastleSeen)) {
                out.push_back(board.castleL<isWhite>());
            }
        }
        if constexpr (wR) {
            if (res.checkCount == 0 && !((res.enemySeen | occ) & wRCastleSeen)) {
                out.push_back(board.castleR<isWhite>());
            }
        }
    } else {
        if constexpr (bL) {
            if (res.checkCount == 0 && !((res.enemySeen | occ) & bLCastleSeen)) {
                out.push_back(board.castleL<isWhite>());
            }
        }
        if constexpr (bR) {
            if (res.checkCount == 0 && !((res.enemySeen | occ) & bRCastleSeen)) {
                out.push_back(board.castleR<isWhite>());
            }
        }
    }

    return out;
}