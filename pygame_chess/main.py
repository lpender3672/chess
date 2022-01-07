import json
import pygame
import move
import data
import gui
import socket, pickle
from data import v2

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((socket.gethostname(), 25565))
s.listen(5)

HEADERSIZE = 10

players = []


while len(players) < 2:
    clientsocket, address = s.accept()

    ip = address[0]
    port = address[1]
    print(f'connection established with {ip} at port {port}')
    client = data.client(clientsocket, ip)
    players.append(client)




with open("data.txt") as dat:
  dat = json.load(dat)
  piecedict = dat["piecedict"]
  boardlayout = dat["boardlayout"]

pygame.init()


board = data.board(boardlayout, piecedict)
displayscale = 100

players[0].send(HEADERSIZE, pickle.dumps(["hello","s1"]))
players[1].send(HEADERSIZE, pickle.dumps(["hello","s2"]))


boardImg = pygame.image.load('board.png')
piecesImg = pygame.image.load('pieces.png')
takeImg = pygame.image.load('takeImg.png')
moveImg = pygame.image.load('moveImg.png')
checkImg = pygame.image.load('incheck.png')

boardImg = pygame.transform.scale(boardImg, (8 * displayscale, 8 * displayscale))
piecesImg = pygame.transform.scale(piecesImg, (6 * displayscale, 2 * displayscale))
takeImg = pygame.transform.scale(takeImg, (int(0.5 * displayscale), int(0.5 * displayscale)))
moveImg = pygame.transform.scale(moveImg, (int(0.5 * displayscale), int(0.5 * displayscale)))
checkImg = pygame.transform.scale(checkImg, (displayscale, displayscale))

clock = pygame.time.Clock()
crashed = False
backtrack = True

offset = v2(0, 0)
gameDisplay = pygame.display.set_mode((8 * displayscale, 8 * displayscale))
pygame.display.set_caption('Chess')

pieceImgdict = gui.get_pieceImgdict(displayscale, piecedict)

colours = {0: "white", 1: "black"}
other = {"white": "black", "black": "white"}
pawndirection = {"white": -1, "black": 1}

gamestage = 0

gameDisplay.fill((255, 255, 255))
gameDisplay.blit(boardImg, (offset.x, offset.y))

gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

moves = []
deselect = False
pawnswap = False

v2_piece = v2(0,0)
v2_movepiece = v2(0,0)
v2_oldpiece = v2(0,0)


piecestaken = {"black": [], "white": []}

while not crashed:
    command = ""
    colour = colours[gamestage % 2]



    command = players[gamestage%2].recieve(HEADERSIZE)

    if command != "":


        x,y = int(command[0]) , int(command[1])
        print(f'client {gamestage % 2} clicked at {x, y} in gamestage {gamestage, deselect}')

        v2_piece = v2(x, y)
        piece = board.getpiece(v2_piece)

        if pawnswap:


            #gameDisplay.fill((255, 255, 255))
            #gameDisplay.blit(boardImg, (offset.x, offset.y))
            players[0].send(HEADERSIZE, pickle.dumps(["clearboard"]))
            players[1].send(HEADERSIZE, pickle.dumps(["clearboard"]))

            # v2_oldpiece
            oldpiece = board.getpiece(v2_oldpiece)
            d = pawndirection[oldpiece.colour]

            piecechoices = [v2(v2_movepiece.x, v2_movepiece.y + (0 * d)), v2(v2_movepiece.x, v2_movepiece.y + (1 * d)),
                            v2(v2_movepiece.x, v2_movepiece.y + (2 * d)), v2(v2_movepiece.x, v2_movepiece.y + (3 * d))]

            pieceids = {0: "8", 1: "10", 2: "11", 3: "9", 4: "3", 5: "5", 6: "4", 7: "2"}
            pawnids = {"white": "6", "black": "12"}

            if v2_piece.inlist(piecechoices):  # if clicked on one of the options

                movepiece = board.getpiece(v2_movepiece)

                if movepiece.name != "":  # if movement spot is a enemy piece
                    piecestaken[colour].append({"gamestage" : gamestage ,"name" : piece.name})  # add to piece to the players taken pile
                    print(piecestaken)

                id = pieceids[v2_piece.y]

                board.overwritepos(v2_oldpiece, data.piece(piecedict, "0"))  # set the pawns old place to empty
                board.overwritepos(v2_movepiece, data.piece(piecedict, id))  # set the pawns new place to the option
                board.saveboard(gamestage)

                players[0].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                players[1].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                #gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

                if oldpiece.colour != "none":
                    kingpos = board.getkingpos(other[oldpiece.colour])

                    if move.checkpos(board, kingpos):
                        print(other[oldpiece.colour], " is in check")
                        board.check = True
                        if move.checkcheckmate(board, kingpos):
                            print("checkmate,", oldpiece.colour, "has won")
                    else:
                        board.check = False
                        if move.checkcheckmate(board, kingpos):
                            print("stalemate its a draw!")

                gamestage += 1

            else:  # if clicked off
                board.overwritepos(v2_oldpiece, data.piece(piecedict, pawnids[oldpiece.colour]))

                players[0].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                players[1].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                #gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

                pawnswap = False
                deselect = True

            if board.check:
                gameDisplay.blit(checkImg, (kingpos.x * displayscale + offset.x, kingpos.y * displayscale + offset.y))

            pawnswap = False
            deselect = False
            # replace v2_oldpiece with newpiece id

        # all the above just to do stupid pawn piece swapping

        else:

            if deselect:  # clicked off piece

                #gameDisplay.fill((255, 255, 255))
                #gameDisplay.blit(boardImg, (offset.x, offset.y))




                if v2_piece.inlist(moves):  # if clicked on a movement spot

                    players[0].send(HEADERSIZE, pickle.dumps(["clearboard"]))
                    players[1].send(HEADERSIZE, pickle.dumps(["clearboard"]))
                    print("sending clear board to clients 1 and 2")

                    oldpiece = board.getpiece(v2_oldpiece)
                    v2_movepiece = v2_piece

                    if oldpiece.name == "pawn" and v2_piece.y == 0:
                        #gui.displaypawnswap(displayscale, pieceImgdict, piecesImg, v2_piece, gameDisplay, offset)

                        players[gamestage % 2].send(HEADERSIZE, pickle.dumps(["displaypawnpromo", [v2_piece.x, v2_piece.y]]))

                        pawnswap = True
                        continue
                    if oldpiece.name == "pawn" and v2_piece.y == 7:
                        #gui.displaypawnswap(displayscale, pieceImgdict, piecesImg, v2_piece, gameDisplay, offset)

                        players[gamestage % 2].send(HEADERSIZE, pickle.dumps(["displaypawnpromo", [v2_piece.x, v2_piece.y]]))

                        pawnswap = True
                        continue

                    if oldpiece.name == "pawn" and v2_piece.y == v2_oldpiece.y - (2 * pawndirection[oldpiece.colour]):

                        board.pawnenpassan = True
                        board.pawnenpassanpos = v2_piece
                    else:
                        board.pawnenpassan = False

                    if piece.name != "":  # if movement spot is a enemy piece
                        piecestaken[colour].append({"gamestage" : gamestage ,"name" : piece.name})  # add to piece to the players taken pile
                        print(piecestaken)

                    board.overwritepos(v2_oldpiece, data.piece(piecedict, "0"))
                    board.overwritepos(v2_piece, data.piece(piecedict, oldpiece.id))  # replace piece

                    if oldpiece.name == "pawn" and piece.name == "" and v2_piece.y == v2_oldpiece.y - pawndirection[
                        oldpiece.colour] and v2_piece.x == v2_oldpiece.x + 1:  # pawn passan right
                        v2_oldpawn = v2(v2_piece.x, v2_piece.y + pawndirection[oldpiece.colour])
                        piecestaken[colour].append({"gamestage" : gamestage ,"name" : board.getpiece(v2_oldpawn).name})  # add to piece to the players taken pile
                        print(piecestaken)
                        board.overwritepos(v2_oldpawn, data.piece(piecedict, "0"))
                        board.pawnenpassandone = False
                    if oldpiece.name == "pawn" and piece.name == "" and v2_piece.y == v2_oldpiece.y - pawndirection[
                        oldpiece.colour] and v2_piece.x == v2_oldpiece.x - 1:  # pawn passan left
                        v2_oldpawn = v2(v2_piece.x, v2_piece.y + pawndirection[oldpiece.colour])
                        piecestaken[colour].append({"gamestage" : gamestage ,"name" : board.getpiece(v2_oldpawn).name})  # add to piece to the players taken pile
                        print(piecestaken)
                        board.overwritepos(v2_oldpawn, data.piece(piecedict, "0"))
                        board.pawnenpassandone = False

                    if oldpiece.name == "king" and v2_piece.x + 2 == v2_oldpiece.x:  ## rook king swap left
                        v2_rook = v2(v2_piece.x - 1, v2_piece.y)
                        rook = board.getpiece(v2_rook)
                        v2_newrook = v2(v2_oldpiece.x - 1, v2_piece.y)

                        board.overwritepos(v2_rook, data.piece(piecedict, "0"))
                        board.overwritepos(v2_newrook, data.piece(piecedict, rook.id))

                    if oldpiece.name == "king" and v2_piece.x - 2 == v2_oldpiece.x:  ## rook king swap right
                        v2_rook = v2(v2_piece.x + 2, v2_piece.y)
                        rook = board.getpiece(v2_rook)
                        v2_newrook = v2(v2_oldpiece.x + 1, v2_piece.y)

                        board.overwritepos(v2_rook, data.piece(piecedict, "0"))
                        board.overwritepos(v2_newrook, data.piece(piecedict, rook.id))



                    board.saveboard(gamestage)

                    # test check
                    if oldpiece.colour != "none":
                        kingpos = board.getkingpos(other[oldpiece.colour])

                        if move.checkpos(board, kingpos):
                            print(other[oldpiece.colour], " is in check")
                            board.check = True
                            if move.checkcheckmate(board, kingpos):
                                print("checkmate,", oldpiece.colour, "has won")
                        else:
                            board.check = False
                            if move.checkcheckmate(board, kingpos):
                                print("stalemate its a draw!")

                    # if in check test if player has any possible move

                    gamestage += 1  # next player

                if board.check:
                    gameDisplay.blit(checkImg, (kingpos.x * displayscale + offset.x, kingpos.y * displayscale + offset.y))

                #gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)  # refresh board

                players[0].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                players[1].send(HEADERSIZE, pickle.dumps(["displayboard", board.board]))
                print("sending display board to clients 1 and 2")

                deselect = False  # allows another piece's movement to be viewed

            if not deselect and piece.colour == colour:
                moves = move.getmoves(board, v2(x, y))
                moves = move.correctmoves(board, v2(x, y), moves)

                #gui.displayMoves(displayscale, moveImg, takeImg, gameDisplay, board, moves, offset)


                players[gamestage % 2].send(HEADERSIZE, pickle.dumps(["displaymove", moves]))

                v2_oldpiece = v2(x, y)
                deselect = True

    pygame.display.update()
    clock.tick(60)

pygame.quit()
quit()

## while true
## player n % 2 's go
## get mouse, if clciked on player n%2's colour piece
## get mouse pos//100 and run moves on that pos
## display possible moves on screen
## if clicked a possible move spot
## move piece to that spot
## increment player n