from data import v2
import pygame

def get_pieceImgdict(scale, piecedict):
    pieceImgdict = {"0" : (-98, -96, 100, 100)}

    for key in piecedict:
        i = int(key)
        pieceImgdict[str(i+1)] = ((i%6) * scale + 2, (i//6) * scale + 4 ,scale,scale)
    return pieceImgdict

def displayBoard(scale, pieceImgdict, board, gameDisplay, piecesImg, offsetpos = v2(0,0)):

    for y in range(len(board.board)):
        for x in range(len(board.board[y])):
            piece = board.getpiece(v2(x,y))
            bounds = pieceImgdict[piece.id]

            gameDisplay.blit(piecesImg, (x*scale + offsetpos.x,y*scale + offsetpos.y), bounds)

def displayMoves(scale, moveImg, takeImg, gameDisplay, board, moves, offsetpos = v2(0,0)):
    for move in moves:

        piece = board.getpiece(move)

        if piece.name == "":
            gameDisplay.blit(moveImg, (move.x*scale + (0.25 * scale) + offsetpos.x, move.y*scale + (0.25 * scale) + offsetpos.y))
        else:
            gameDisplay.blit(takeImg, (move.x*scale + (0.25 * scale) + offsetpos.x, move.y*scale + (0.25 * scale) + offsetpos.y))

def displaypawnswap(scale, pieceImgdict, pieceImg, pos, gameDisplay, offsetpos):
    if pos.y == 7:
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y - 0) * scale + offsetpos.y), pieceImgdict["2"]) # black queen at pos
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y - 1) * scale + offsetpos.y), pieceImgdict["4"]) # black horse at pos.y+1
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y - 2) * scale + offsetpos.y), pieceImgdict["5"]) # black rook at pos.y+2
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y - 3) * scale + offsetpos.y), pieceImgdict["3"]) # black bishop at pos.y+3
    if pos.y == 0:
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y + 0) * scale + offsetpos.y), pieceImgdict["8"])  # white queen at pos
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y + 1) * scale + offsetpos.y), pieceImgdict["10"])  # white horse at pos.y-1
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y + 2) * scale + offsetpos.y), pieceImgdict["11"])  # white rook at pos.y-2
        gameDisplay.blit(pieceImg, (pos.x * scale + offsetpos.x, (pos.y + 3) * scale + offsetpos.y), pieceImgdict["9"])  # white bishop at pos.y-3


def print_array(board):

    for row in board:
        rowstr = ""
        for piece in row:
            rowstr +=  piece.name + "       "[0:7-len(piece.name)]
        print(rowstr)

def print_moves(moves):
    for move in moves:
        print(move.x, move.y)