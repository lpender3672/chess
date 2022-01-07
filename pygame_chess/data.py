import socket, pickle, select

class v2(object):
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def zero(self):
        self.x = 0
        self.y = 0

    def inlist(self, list):
        for i in list:
            if i.x == self.x and i.y == self.y:
                return True
        return False

    def equal(self, pos):
        if pos.x == self.x and pos.y == self.y:
            return True
        return False


class server(object):
    def __init__(self, serversocket, buffersize):
        self.serversocket = serversocket
        self.buffersize = buffersize

    def send(self, HEADERSIZE, msg):
        msg = bytes(f'{len(msg):<{HEADERSIZE}}', 'utf-8') + msg
        self.serversocket.send(msg)

    def recieve(self, HEADERSIZE):
        full_msg = b''
        new_msg = True
        msg_recived = False

        self.serversocket.setblocking(1)
        ready = select.select([self.serversocket], [], [], 0.1)

        if ready[0]:
            while not msg_recived:

                msg = self.serversocket.recv(self.buffersize)
                if new_msg:
                    msglen = int(msg[:HEADERSIZE])
                    new_msg = False

                full_msg += msg

                if len(full_msg) - HEADERSIZE == msglen:
                    d = pickle.loads(full_msg[HEADERSIZE:])
                    msg_recived = True

                    return d
        else:
            return ""


class piece(object):
    def __init__(self, piecedict, id):
        self.name = piecedict[id]["name"]
        self.colour = piecedict[id]["colour"]
        self.unicode = piecedict[id]["unicode"]
        self.id = id


class board(object):
    def __init__(self, layout, piecedict, imported = False):
        self.startlayout = layout
        self.history = {}
        self.piecedict = piecedict
        self.check = False
        if imported:
            self.board = layout
        else:
            board = []

            for row in layout:
                board_row = []
                for id in row:
                    board_row.append(piece(self.piecedict, id))
                board.append(board_row)
            self.board = board


    def idmap(self):
        idmap = []
        for y in range(len(self.board)):
            idmap_row = []
            for x in range(len(self.board[y])):
                idmap_row.append(self.board[y][x].id)
            idmap.append(idmap_row)
        return idmap


    def getpiece(self, pos):
        return self.board[pos.y][pos.x]
        ## get piece from x,y

    def overwritepos(self, pos, piece):
        self.board[pos.y][pos.x] = piece

    def saveboard(self, gamestage):
        self.history[gamestage] = self.idmap()


    def getkingpos(self, colour):
        for y in range(len(self.board)):
            for x in range(len(self.board[y])):
                if self.board[y][x].name == "king" and self.board[y][x].colour == colour:
                    return v2(x,y)


    def getpiecespos(self, colour, name=""):
        piecespos = []
        for y in range(len(self.board)):
            for x in range(len(self.board[y])):
                if name != "":
                    if self.board[y][x].name == name and self.board[y][x].colour == colour:
                        piecespos.append(v2(x,y))
                else:
                    if self.board[y][x].colour == colour:
                        piecespos.append(v2(x, y))
        return piecespos


    def unmovedpiece(self, pos):
        for key in self.history:
            if self.history[key][pos.y][pos.x] != self.startlayout[pos.y][pos.x]:
                return False
        return True

