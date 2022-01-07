import socket, pickle
import pygame, json
import data
from data import v2
import gui

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((socket.gethostname(), 25565))
print(socket.gethostname())


HEADERSIZE = 10

server = data.server(s, 4096)

#server.send(HEADERSIZE, pickle.dumps(["hello","server from client 1"]))


#while True:
#    msg1 = server1.recieve(HEADERSIZE)
#    print(msg1)
#    msg2 = server2.recieve(HEADERSIZE)
#    print(msg2)


with open("data.txt") as dat:
  dat = json.load(dat)
  piecedict = dat["piecedict"]
  boardlayout = dat["boardlayout"]

pygame.init()

board = data.board(boardlayout, piecedict)
displayscale = 100


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

gameDisplay.fill((255, 255, 255))
gameDisplay.blit(boardImg, (offset.x, offset.y))

gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)


while not crashed:
    event = pygame.event.poll()
    command = ""




    if event.type == pygame.QUIT:
        # dc
        crashed = True

    elif event.type == pygame.MOUSEBUTTONDOWN:
        x, y = pygame.mouse.get_pos()
        x = (x - offset.x) // displayscale
        y = (y - offset.y) // displayscale
        if x in range(0,8) and y in range(0,8):
            server.send( HEADERSIZE, pickle.dumps([x,y]))

    command = server.recieve(HEADERSIZE)
    if  command != "":  # update


        if command[0] == "clearboard":
            print("clear board")

            gameDisplay.fill((255, 255, 255))
            gameDisplay.blit(boardImg, (offset.x, offset.y))


        if command[0] == "displayboard":  # update type
            print("display board")

            layout = command[1]
            board = data.board(layout, piecedict, True)
            pygame.display.update()

            gameDisplay.fill((255, 255, 255))
            gameDisplay.blit(boardImg, (offset.x, offset.y))

            gui.displayBoard(displayscale, pieceImgdict, board, gameDisplay, piecesImg, offset)
            pygame.display.update()

        if command[0] == "displaymove":
            print("display move")

            moves = command[1]

            gui.displayMoves(displayscale, moveImg, takeImg, gameDisplay, board, moves)
            pygame.display.update()

        if command[0] == "displaypawnpromo":
            print("display pawn promo")

            pos = command[1]
            v2pos = v2(pos[0], pos[1])

            gui.displaypawnswap(displayscale, pieceImgdict, piecesImg, v2pos, gameDisplay, offset)
            pygame.display.update()




    clock.tick(60)




# update


# if send an update;
# update.type =



