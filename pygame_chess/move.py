import data
from data import v2
import gui


def vectorflipper(board, pos, mov,
                  infinite=False):  # takes a movement v2 and a pos v2 and displays the movement in each direction
    unit_identity_list = [v2(1, 1), v2(-1, 1), v2(1, -1),
                          v2(-1, -1)]  # around the pos and then checks if the output pos is valid to be a basic move
    pos_piece = board.getpiece(pos)
    repeat = 2
    if infinite:
        repeat = 8
    moves = []
    for unit_identity in unit_identity_list:
        for i in range(1, repeat):
            x = i * mov.x * unit_identity.x + pos.x
            y = i * mov.y * unit_identity.y + pos.y
            if x >= 0 and x <= 7 and y >= 0 and y <= 7:
                mov_piece = board.getpiece(v2(x, y))
                if mov_piece.colour == pos_piece.colour:
                    break
                elif mov_piece.name == "":
                    moves.append(v2(x, y))
                elif mov_piece.colour != pos_piece.colour:
                    moves.append(v2(x, y))
                    break
    return moves


class piecemoves:
    def rook(board, pos):
        moves = []
        moves += vectorflipper(board, pos, v2(0, 1), True)
        moves += vectorflipper(board, pos, v2(1, 0), True)
        return moves

    def knight(board, pos):
        moves = []
        moves += vectorflipper(board, pos, v2(2, 1))
        moves += vectorflipper(board, pos, v2(1, 2))
        return moves

    def bishop(board, pos):
        moves = []
        moves += vectorflipper(board, pos, v2(1, 1), True)
        return moves

    def queen(board, pos):
        moves = []
        moves += vectorflipper(board, pos, v2(1, 1), True)
        moves += vectorflipper(board, pos, v2(0, 1), True)
        moves += vectorflipper(board, pos, v2(1, 0), True)
        return moves

    def king(board, pos):
        moves = []
        moves += vectorflipper(board, pos, v2(1, 1))
        moves += vectorflipper(board, pos, v2(0, 1))
        moves += vectorflipper(board, pos, v2(1, 0))

        leftrookdic = {"white": v2(0, 0), "black": v2(0, 7)}
        rightrookdic = {"white": v2(7, 0), "black": v2(7, 7)}
        piece = board.getpiece(pos)

        leftrookpos = leftrookdic[piece.colour]
        rightrookpos = rightrookdic[piece.colour]

        if board.unmovedpiece(leftrookpos) and board.unmovedpiece(pos) and not board.check:
            v2_rook1plus = v2(leftrookpos.x + 1, leftrookpos.y)
            v2_rook2plus = v2(leftrookpos.x + 2, leftrookpos.y)

            if board.getpiece(v2_rook1plus).name == "" and board.getpiece(v2_rook2plus).name == "":
                moves.append(v2_rook1plus)

        if board.unmovedpiece(rightrookpos) and board.unmovedpiece(pos) and not board.check:
            v2_rook1minus = v2(rightrookpos.x - 1, rightrookpos.y)
            v2_rook2minus = v2(rightrookpos.x - 2, rightrookpos.y)
            v2_rook3minus = v2(rightrookpos.x - 3, rightrookpos.y)

            if board.getpiece(v2_rook1minus).name == "" and board.getpiece(v2_rook2minus).name == "" and board.getpiece(
                    v2_rook3minus).name == "":
                moves.append(v2_rook2minus)

        return moves

    def pawn(board, pos):
        colourmap = {"white": 1, "black": -1}
        piece = board.getpiece(pos)
        d = colourmap[piece.colour]  ## d is + or - based on colour (prawns can only move one direction)
        moves = []

        y1d = (pos.y + (1 * d))
        y2d = (pos.y + (2 * d))
        v2_y1d = v2(pos.x, y1d)
        v2_y2d = v2(pos.x, y2d)

        v2_xplus = v2(pos.x + 1, pos.y + d)
        v2_xminus = v2(pos.x - 1, pos.y + d)

        if y1d in range(0, 8) and board.getpiece(v2_y1d).name == "":  ## if piece.pos+(0, 1d) is empty move is possible
            moves.append(v2_y1d)
        if y2d in range(0, 8) and board.getpiece(v2_y1d).name == "" and board.getpiece(v2_y2d).name == "" and pos.y in [
            1, 6]:  ## if piece.pos+(0, 2d) is empty and piece is at y = 1 or y = 6 move is possible
            moves.append(v2_y2d)
        if y1d in range(0, 8) and pos.x != 7 and board.getpiece(v2_xplus).colour != piece.colour and board.getpiece(
                v2_xplus).name != "":  ## if piece.pos+(1,d) is not empty and not its own colour move is possibl
            if board.getpiece(v2_xplus).name != "king":
                moves.append(v2_xplus)
        if y1d in range(0, 8) and pos.x != 0 and board.getpiece(v2_xminus).colour != piece.colour and board.getpiece(
                v2_xminus).name != "":  ## if piece.pos+(-1,d) is not empty and not its own colour move is possible
            if board.getpiece(v2_xminus).name != "king":
                moves.append(v2_xminus)

        # pawn en passan
        if board.pawnenpassan:

            board.pawnenpassandone = False

            v2_py1d = v2(board.pawnenpassanpos.x, board.pawnenpassanpos.y + d)

            v2_pxplus = v2(board.pawnenpassanpos.x + 1, board.pawnenpassanpos.y)
            v2_pxminus = v2(board.pawnenpassanpos.x - 1, board.pawnenpassanpos.y)

            if v2_py1d.y in range(0, 8) and v2_pxminus.equal(pos) and board.getpiece(
                    v2_py1d).name == "":  # needs a better simulate possible to work better
                moves.append(v2_py1d)
                board.pawnenpassandone = True
            elif v2_py1d.y in range(0, 8) and v2_pxplus.equal(pos) and board.getpiece(v2_py1d).name == "":
                moves.append(v2_py1d)
                board.pawnenpassandone = True

        return moves


def getmoves(board, pos, name=""):
    if name == "":
        name = board.getpiece(pos).name

    if name == "rook":
        return piecemoves.rook(board, pos)
    elif name == "knight":
        return piecemoves.knight(board, pos)
    elif name == "bishop":
        return piecemoves.bishop(board, pos)
    elif name == "queen":
        return piecemoves.queen(board, pos)
    elif name == "king":
        return piecemoves.king(board, pos)
    elif name == "pawn":
        return piecemoves.pawn(board, pos)


def correctmoves(board, pos, moves):
    finalmoves = []
    for move in moves:

        if simulatepossible(board, pos, move):
            finalmoves.append(move)
    return finalmoves


def checkpos(board, pos):  ## checks if a pos is in check
    piece = board.getpiece(pos)
    other = {"white": "black", "black": "white"}
    colourmap = {"white": 1, "black": -1}
    piecenames = ["king", "queen", "bishop", "knight", "rook"]
    for piecename in piecenames:  # checks every pieces moves at the pos for a piece of opposite colour, so the opposite colour could move there too, so its in check
        moves = getmoves(board, pos, piecename)  # moves works because the pos being checked for in check

        for move in moves:  # each move
            if board.getpiece(
                    move).name == piecename:  # if piece at move location same name as the piece being checked for check
                return True

    # next is for stupid pawns
    if piece.colour != "none":
        colour = other[piece.colour]
        d = colourmap[colour]
        pieces = board.getpiecespos(colour, "pawn")  # gets every pos of a pawn on the board
        for piec in pieces:

            v2_xplus = v2(piec.x + 1, piec.y + d)  # the pawns take moves
            v2_xminus = v2(piec.x - 1, piec.y + d)

            if v2_xplus.equal(pos):  ## doesnt matter if the moves are out of bounds of the board
                return True
            if v2_xminus.equal(pos):
                return True

    # go through all pawns on the board

    return False
    # return true if piece of other colour can move here


def simulatepossible(board, frompos,
                     topos):  # checks if a move being done will result in the king being in check after the move is done

    frompiece = board.getpiece(frompos)
    topiece = board.getpiece(topos)

    colour = board.getpiece(frompos).colour

    board.overwritepos(frompos, data.piece(board.piecedict, "0"))
    board.overwritepos(topos, data.piece(board.piecedict, frompiece.id))  # replace piece

    if board.pawnenpassandone and frompos.x != topos.x:
        pawnpiece = board.getpiece(board.pawnenpassanpos)
        if pawnpiece.name == "pawn":
            board.overwritepos(board.pawnenpassanpos, data.piece(board.piecedict, "0"))

    kingpos = board.getkingpos(colour)
    # gets the kings position (if the king is the piece being tested when its moved it gets its new pos)
    try:
        kingnotincheck = not checkpos(board, kingpos)
    except:
        print(kingpos.x, kingpos.y)

    kingnotincheck = not checkpos(board, kingpos)

    board.overwritepos(topos, data.piece(board.piecedict, topiece.id))
    board.overwritepos(frompos, data.piece(board.piecedict, frompiece.id))

    if board.pawnenpassandone and frompos.x != topos.x and pawnpiece.name == "pawn":
        board.overwritepos(board.pawnenpassanpos, data.piece(board.piecedict, pawnpiece.id))

    if kingnotincheck:
        return True

    return False  # this code is quite messy but works just fine (writing move, checking if king in check then unwriting move)


def checkcheckmate(board, kingpos):
    piece = board.getpiece(kingpos)
    pieces = board.getpiecespos(piece.colour)

    for pos in pieces:
        moves = getmoves(board, pos)
        moves = correctmoves(board, pos, moves)
        if len(moves) != 0:
            return False
    return True


def getallmoves(board, colour):
    board.getpiecespos()

    pieces = ["king", "queen", "rook", "knight", "bishop", "pawn"]

    poses = []
    for piece in pieces:
        poses.append(board.getpiecespos(colour, name=piece))

    moves = []
    for pos in poses:
        posmoves = getmoves(board, pos)
        moves.append(correctmoves(board, pos, posmoves))

    return moves

# 200