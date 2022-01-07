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





class piece(object):
    def __init__(self, piecedict, id):
        self.name = piecedict[id]["name"]
        self.colour = piecedict[id]["colour"]
        self.unicode = piecedict[id]["unicode"]
        self.id = id

class client(object):
    def __init__(self, clientsocket,  ip):
        self.clientsocket = clientsocket
        self.ip = ip
        self.buffersize = 8192

    def send(self, HEADERSIZE, msg):
        msg = bytes(f'{len(msg):<{HEADERSIZE}}', 'utf-8') + msg
        self.clientsocket.send(msg)

    def recieve(self ,HEADERSIZE):
        full_msg = b''
        new_msg = True
        msg_recived = False

        self.clientsocket.setblocking(1)
        ready = select.select([self.clientsocket], [], [], 0.01)

        if ready[0]:
            while not msg_recived:

                msg = self.clientsocket.recv(self.buffersize)
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






class board(object):
    def __init__(self, layout, piecedict):
        self.startlayout = layout
        self.history = {}
        self.piecedict = piecedict
        self.check = False
        self.pawnenpassan = False
        self.pawnenpassandone = False
        self.pawnenpassanpos = v2(0,0)

        self.board = self.piecemap(layout)

        self.saveboard(-1)

    def piecemap(self, layout):
        bord = []

        for row in layout:
            bord_row = []
            for id in row:
                bord_row.append(piece(self.piecedict, id))
            bord.append(bord_row)

        return bord

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

    def loadboard(self, gamestage):
        layout = self.history[gamestage]
        self.board = self.piecemap(layout)



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

