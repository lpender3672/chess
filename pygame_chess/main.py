import json
import pygame
import move
import data
import gui
from stockfish import Stockfish
from data import v2

engine = Stockfish("stockfish")

with open("data.txt") as dat:
    dat = json.load(dat)
    piecedict = dat["piecedict"]
    boardlayout = dat["boardlayout"]

pygame.init()

displayscale = 100

board = data.board(boardlayout, piecedict)

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

againststockfish = False
playercolour = "black"

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


board.piecestaken = {"black": [], "white": []}

engine.set_skill_level(9)

while not crashed:
    colour = colours[gamestage % 2]

    if againststockfish and colour == other[playercolour]:


        engine.set_position(board.gethistoryhash())

        movehash = engine.get_best_move_time(1000)


        enginemove = data.move(board,v2.zero, v2.zero, hash = movehash)
        if enginemove.fropiece == "pawn":
            if enginemove.to.y == 7:
                enginemove.createpiece = data.piece(piecedict, "2")
            if enginemove.to.y == 0:
                enginemove.createpiece = data.piece(piecedict, "8")

        enginemove.perform(board)

        if enginemove.topiece.name != "":
            board.piecestaken[colour].append({"gamestage": gamestage, "name": enginemove.topiece.name})
            print(board.piecestaken)

        board.history.append(enginemove)


        gameDisplay.fill((255, 255, 255))
        gameDisplay.blit(boardImg, (offset.x, offset.y))
        gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

        kingpos = board.getkingpos(playercolour)
        if move.checkpos(board, kingpos):
            gameDisplay.blit(checkImg, (kingpos.x * displayscale + offset.x, kingpos.y * displayscale + offset.y))
            print(other[colour], " is in check")
            board.check = True
            if move.checkcheckmate(board, other[colour], kingpos):
                print("checkmate,", colour, "has won")
                if other[colour] == other[playercolour]:
                    crashed = True

        else:
            board.check = False
            if move.checkcheckmate(board, other[colour], kingpos):
                print("stalemate its a draw!")
                if other[colour] == other[playercolour]:
                    crashed = True



        gamestage+=1
        continue

    waitevent = pygame.event.wait()

    if waitevent.type == pygame.QUIT:
        crashed = True

    if waitevent.type == pygame.KEYDOWN:

        if waitevent.key == pygame.K_c:

            againststockfish = not againststockfish
            continue


        if waitevent.key == pygame.K_SPACE and gamestage >= 1 and not againststockfish:
            # roll back
            gameDisplay.fill((255, 255, 255))
            gameDisplay.blit(boardImg, (offset.x, offset.y))

            gamestage -= 1
            board.history[len(board.history) - 1].unperform(board)
            del board.history[len(board.history) - 1]

            for key in board.piecestaken:
                takenpieces = board.piecestaken[key]
                for i in range(len(takenpieces)):
                    if takenpieces[i]["gamestage"] >= gamestage:
                        del takenpieces[i]


        gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)


    elif waitevent.type == pygame.MOUSEBUTTONDOWN:
        x, y = pygame.mouse.get_pos()
        x = (x - offset.x) // displayscale
        y = (y - offset.y) // displayscale
        if not (x in range(0, 8) and y in range(0, 8)):  ## if mouse out of bounds dont get pos
            continue

        v2_piece = v2(x, y)
        piece = board.getpiece(v2_piece)

        if pawnswap:

            gameDisplay.fill((255, 255, 255))
            gameDisplay.blit(boardImg, (offset.x, offset.y))

            # v2_oldpiece
            oldpiece = board.getpiece(v2_oldpiece)
            d = pawndirection[oldpiece.colour]



            piecechoices = [v2(v2_movepiece.x, v2_movepiece.y + (0 * d)),
                            v2(v2_movepiece.x, v2_movepiece.y + (1 * d)),
                            v2(v2_movepiece.x, v2_movepiece.y + (2 * d)),
                            v2(v2_movepiece.x, v2_movepiece.y + (3 * d))]

            pieceids = {0: "8", 1: "10", 2: "11", 3: "9", 4: "3", 5: "5", 6: "4", 7: "2"}
            pawnids = {"white": "6", "black": "12"}

            if v2_piece.inlist(piecechoices):  # if clicked on one of the options
                promotionpiece = data.piece(piecedict, pieceids[v2_piece.y])
                mov = data.move(board, v2_movepiece, v2_oldpiece, createpiece= promotionpiece)

                movepiece = board.getpiece(v2_movepiece)

                if movepiece.name != "":  # if movement spot is a enemy piece
                    board.piecestaken[colour].append({"gamestage" : gamestage ,"name" : piece.name})  # add to piece to the players taken pile
                    print(board.piecestaken)


                mov.perform(board)
                engine.set_position([mov.hash])
                board.history.append(mov)
                #board.overwritepos(v2_oldpiece, data.piece(piecedict, "0"))  # set the pawns old place to empty
                #board.overwritepos(v2_movepiece, data.piece(piecedict, id))  # set the pawns new place to the option


                gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

                if oldpiece.colour != "none":
                    kingpos = board.getkingpos(other[oldpiece.colour])

                    if move.checkpos(board, kingpos):
                        print(other[oldpiece.colour], " is in check")
                        board.check = True
                        if move.checkcheckmate(board, other[colour], kingpos):
                            print("checkmate,", oldpiece.colour, "has won")
                    else:
                        board.check = False
                        if move.checkcheckmate(board, other[colour], kingpos):
                            print("stalemate its a draw!")

                gamestage += 1

            else:  # if clicked off
                # UNPERFORM MOVE

                gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)

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

                gameDisplay.fill((255, 255, 255))
                gameDisplay.blit(boardImg, (offset.x, offset.y))

                if v2_piece.inmoveslist(moves):  # if clicked on a movement spot
                    mov = move.searchmove(moves, v2_piece)
                    oldpiece = board.getpiece(v2_oldpiece)
                    v2_movepiece = v2_piece

                    if oldpiece.name == "pawn" and v2_piece.y == 0:
                        gui.displaypawnswap(displayscale, pieceImgdict, piecesImg, v2_piece, gameDisplay, offset)
                        pawnswap = True
                        continue
                    if oldpiece.name == "pawn" and v2_piece.y == 7:
                        gui.displaypawnswap(displayscale, pieceImgdict, piecesImg, v2_piece, gameDisplay, offset)
                        pawnswap = True
                        continue
                    if oldpiece.name == "pawn" and v2_piece.y == v2_oldpiece.y - (2 * pawndirection[oldpiece.colour]):

                        board.pawnenpassan = True
                        board.pawnenpassanpos = v2_piece
                    else:
                        board.pawnenpassan = False

                    if piece.name != "":  # if movement spot is a enemy piece
                        board.piecestaken[colour].append({"gamestage" : gamestage ,"name" : piece.name})  # add to piece to the players taken pile
                        print(board.piecestaken)

                    ## PERFORM MOVE
                    mov.perform(board)
                    engine.set_position([mov.hash])

                    board.history.append(mov)

                    # test check
                    kingpos = board.getkingpos(other[colour])

                    if move.checkpos(board, kingpos):
                        print(other[colour], " is in check")
                        board.check = True
                        if move.checkcheckmate(board, other[colour], kingpos):
                            print("checkmate,", colour, "has won")
                    else:
                        board.check = False
                        if move.checkcheckmate(board, other[colour], kingpos):
                            print("stalemate its a draw!")

                    # if in check test if player has any possible move

                    gamestage += 1  # next player

                if board.check:
                    gameDisplay.blit(checkImg,(kingpos.x * displayscale + offset.x, kingpos.y * displayscale + offset.y))

                gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)  # refresh board

                deselect = False  # allows another piece's movement to be viewed

            if not deselect and piece.colour == colour:

                moves = move.getmoves(board, v2(x, y))

                moves = move.correctmoves(board,colour,moves)

                gui.displayMoves(displayscale, moveImg, takeImg, gameDisplay, board, moves, offset)

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

