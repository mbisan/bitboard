#include "base.cpp"
#include "lookup.cpp"

#include <string>
#include <cstdint>
#include <vector>

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

template<bool isWhite>
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
    if constexpr (isWhite) {
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

template<GameState state>
std::vector<Board> generateMoves(const Board &board) {
    Pieces self, enemy;
    if constexpr (state.isWhite) {
        self = board.w; enemy = board.b;
    } else {
        self = board.b; enemy = board.w;
    }

    std::vector<Board> out;

    statusReport res = check<state.isWhite>(self, enemy);
    Squares notEnemy = ~res.enemyOcc;
    Squares notSelf = ~res.selfOcc;
    Squares occ = res.selfOcc | res.enemyOcc;
    Squares notpin = ~(res.pinHV | res.pinD);

    // king moves
    {    
        Squares kingReachable = kingMoves[res.kingIndex] & ~res.kingBan & ~res.enemySeen & notSelf;
        BitLoop(kingReachable & notEnemy) { out.push_back(board.pieceMove<state, KING>(_blsi_u64(temp) | self.k)); }
        BitLoop(kingReachable & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, KING>(_blsi_u64(temp) | self.k)); }

        if (res.checkCount > 1) { // only king can move
            return out;
        }
    }

    Squares notselfCheckmask = notSelf & res.checkMask;

    // rook moves
    {
        // pinned
        for (Squares pinnedRooks = self.r & res.pinHV; pinnedRooks; pinnedRooks = _blsr_u64(pinnedRooks)) {
            Squares current = _blsi_u64(pinnedRooks);
            uint64_t pieceIndex = _tzcnt_u64(pinnedRooks);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, ROOK>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, ROOK>(_blsi_u64(temp) | current)); }
        }
        // not pinned
        for (Squares unpinnedRooks = self.r & notpin; unpinnedRooks; unpinnedRooks = _blsr_u64(unpinnedRooks)) {
            Squares current = _blsi_u64(unpinnedRooks);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedRooks);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, ROOK>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, ROOK>(_blsi_u64(temp) | current)); }
        }
    }

    // bishop moves
    {
        // pinned
        for (Squares pinnedBishops = self.r & res.pinD; pinnedBishops; pinnedBishops = _blsr_u64(pinnedBishops)) {
            Squares current = _blsi_u64(pinnedBishops);
            uint64_t pieceIndex = _tzcnt_u64(pinnedBishops);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinD;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, BISHOP>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, BISHOP>(_blsi_u64(temp) | current)); }
        }
        // not pinned
        for (Squares unpinnedBishops = self.r & notpin; unpinnedBishops; unpinnedBishops = _blsr_u64(unpinnedBishops)) {
            Squares current = _blsi_u64(unpinnedBishops);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedBishops);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, BISHOP>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, BISHOP>(_blsi_u64(temp) | current)); }
        }
    }

    // queen moves
    {
        for (Squares queensHV = self.q & res.pinHV; queensHV; queensHV = _blsr_u64(queensHV)) {
            Squares current = _blsi_u64(queensHV);
            uint64_t pieceIndex = _tzcnt_u64(queensHV);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, QUEEN>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, QUEEN>(_blsi_u64(temp) | current)); }
        }
        for (Squares queensD = self.q & res.pinD; queensD; queensD = _blsr_u64(queensD)) {
            Squares current = _blsi_u64(queensD);
            uint64_t pieceIndex = _tzcnt_u64(queensD);
            Squares atk = slide(bishopMoves[pieceIndex], occ, pieceIndex) & notselfCheckmask & res.pinHV;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, QUEEN>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, QUEEN>(_blsi_u64(temp) | current)); }
        }
        for (Squares unpinnedQueens = self.q & notpin; unpinnedQueens; unpinnedQueens = _blsr_u64(unpinnedQueens)) {
            Squares current = _blsi_u64(unpinnedQueens);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedQueens);
            Squares atk = slide(rookMoves[pieceIndex], occ, pieceIndex) & slide(bishopMoves[pieceIndex], occ, pieceIndex);
            atk &= notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, QUEEN>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, QUEEN>(_blsi_u64(temp) | current)); }
        }
    }

    // knight moves
    {
        for (Squares unpinnedKnight = self.n & notpin; unpinnedKnight; unpinnedKnight = _blsr_u64(unpinnedKnight)) {
            Squares current = _blsi_u64(unpinnedKnight);
            uint64_t pieceIndex = _tzcnt_u64(unpinnedKnight);
            Squares atk = knightMoves[pieceIndex] & notselfCheckmask;

            BitLoop(atk & notEnemy) { out.push_back(board.pieceMove<state, KNIGHT>(_blsi_u64(temp) | current)); }
            BitLoop(atk & res.enemyOcc) { out.push_back(board.pieceMoveCapture<state, KNIGHT>(_blsi_u64(temp) | current)); }
        }
    }

    // pawns
    {
        Squares pawnAdvance;
        if constexpr (state.isWhite) pawnAdvance = self.p & ~wPawnLast & ~(occ << 8);
        else pawnAdvance = self.p & ~bPawnLast & ~(occ >> 8);

        Squares pawnPush;
        if constexpr (state.isWhite) pawnPush = pawnAdvance & wPawnStart & ~(occ << 16);
        else pawnPush = pawnAdvance & wPawnStart & ~(occ >> 16);

        Squares pawnCaptureL;
        if constexpr (state.isWhite) pawnCaptureL = self.p & ~wPawnLast & ((res.enemyOcc & notHfile) << 7);
        else pawnCaptureL = self.p & ~wPawnLast & ((res.enemyOcc & notHfile) >> 7);

        Squares pawnCaptureR;
        if constexpr (state.isWhite) pawnCaptureR = self.p & ~wPawnLast & ((res.enemyOcc & notAfile) << 9);
        else pawnCaptureR = self.p & ~wPawnLast & ((res.enemyOcc & notAfile) >> 9);

        Squares pawnAdvancePromote;
        if constexpr (state.isWhite) pawnAdvancePromote = self.p & wPawnLast & ~(occ << 8);
        else pawnAdvancePromote = self.p & bPawnLast & ~(occ >> 8);

        Squares pawnCaptureLpromote;
        if constexpr (state.isWhite) pawnCaptureLpromote = self.p & wPawnLast & ((res.enemyOcc & notHfile) << 7);
        else pawnCaptureLpromote = self.p & wPawnLast & ((res.enemyOcc & notHfile) >> 7);

        Squares pawnCaptureRpromote;
        if constexpr (state.isWhite) pawnCaptureRpromote = self.p & wPawnLast & ((res.enemyOcc & notAfile) << 9);
        else pawnCaptureRpromote = self.p & wPawnLast & ((res.enemyOcc & notAfile) >> 9);

        // not pinned
        BitLoop(pawnAdvance & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 8;
            else final = current << 8;            
            out.push_back(board.pieceMove<state, PAWN>(current | final)); 
        }
        BitLoop(pawnPush & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 16;
            else final = current << 16;
            out.push_back(board.pawnPush<state>(current, current | final)); 
        }
        BitLoop(pawnCaptureL & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 7;
            else final = current << 7;
            out.push_back(board.pieceMoveCapture<state, PAWN>(current | final)); 
        }
        BitLoop(pawnCaptureR & notpin) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 9;
            else final = current << 9;
            out.push_back(board.pieceMoveCapture<state, PAWN>(current | final)); 
        }
        // pinned
        BitLoop(pawnAdvance & res.pinHV) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 8;
            else final = current << 8;            
            if (final & res.pinHV) out.push_back(board.pieceMove<state, PAWN>(current | final)); 
        }
        BitLoop(pawnPush & res.pinHV) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 16;
            else final = current << 16;
            if (final & res.pinHV) out.push_back(board.pawnPush<state>(current, current | final)); 
        }
        BitLoop(pawnCaptureL & res.pinD) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 7;
            else final = current << 7;
            if (final & res.pindD) out.push_back(board.pieceMoveCapture<state, PAWN>(current | final)); 
        }
        BitLoop(pawnCaptureR & res.pinD) { 
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 9;
            else final = current << 9;
            if (final & res.pindD) out.push_back(board.pieceMoveCapture<state, PAWN>(current | final)); 
        }
        // not pinned promotion
        BitLoop(pawnAdvancePromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 8;
            else final = current << 8;
            out.push_back(board.pawnPromote<state, QUEEN>(current, final));
            out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
            out.push_back(board.pawnPromote<state, BISHOP>(current, final));
            out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
        }
        BitLoop(pawnCaptureLpromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 7;
            else final = current << 7;
            out.push_back(board.pawnPromote<state, QUEEN>(current, final));
            out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
            out.push_back(board.pawnPromote<state, BISHOP>(current, final));
            out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
        }
        BitLoop(pawnCaptureRpromote & notpin) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 9;
            else final = current << 9;
            out.push_back(board.pawnPromote<state, QUEEN>(current, final));
            out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
            out.push_back(board.pawnPromote<state, BISHOP>(current, final));
            out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
        }
        // pinned promotion
        BitLoop(pawnAdvancePromote & res.pinHV) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 8;
            else final = current << 8;
            if (final & res.pinHV) {
                out.push_back(board.pawnPromote<state, QUEEN>(current, final));
                out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
                out.push_back(board.pawnPromote<state, BISHOP>(current, final));
                out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
            }
        }
        BitLoop(pawnCaptureLpromote & res.pinD) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 7;
            else final = current << 7;
            if (final & res.pinHV) {
                out.push_back(board.pawnPromote<state, QUEEN>(current, final));
                out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
                out.push_back(board.pawnPromote<state, BISHOP>(current, final));
                out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
            }
        }
        BitLoop(pawnCaptureRpromote & res.pinD) {
            Squares current = _blsi_u64(temp); Squares final;
            if constexpr (state.isWhite) final = current >> 9;
            else final = current << 9;
            if (final & res.pinD) {
                out.push_back(board.pawnPromote<state, QUEEN>(current, final));
                out.push_back(board.pawnPromote<state, ROOK>(current, final)); 
                out.push_back(board.pawnPromote<state, BISHOP>(current, final));
                out.push_back(board.pawnPromote<state, KNIGHT>(current, final));
            }
        }
    }

    // enpassant
    if constexpr (state.ep) {
        Squares epL = board.ep & (self.p >> 1) & ~res.epPin;
        Squares epR = board.ep & (self.p << 1) & ~res.epPin;
        
        Squares enemyPawnBehind;
        if constexpr (state.isWhite) enemyPawnBehind = epL >> 8;
        else enemyPawnBehind = epL << 8;

        if (epL) {
            out.push_back(board.pieceMoveCapture<state, PAWN>(board.ep | epL | enemyPawnBehind));
        }
        if (epR) {
            out.push_back(board.pieceMoveCapture<state, PAWN>(board.ep | epR | enemyPawnBehind));
        }
    }

    // castles
    if constexpr (state.isWhite) {
        if constexpr (state.wL) {
            if (res.checkCount == 0 && !(res.enemySeen & wLCastleSeen)) {
                out.push_back(board.castleL<state>());
            }
        }
        if constexpr (state.wR) {
            if (res.checkCount == 0 && !(res.enemySeen & wRCastleSeen)) {
                out.push_back(board.castleR<state>());
            }
        }
    } else {
        if constexpr (state.bL) {
            if (res.checkCount == 0 && !(res.enemySeen & bLCastleSeen)) {
                out.push_back(board.castleL<state>());
            }
        }
        if constexpr (state.bR) {
            if (res.checkCount == 0 && !(res.enemySeen & bRCastleSeen)) {
                out.push_back(board.castleR<state>());
            }
        }
    }

    return out;
}