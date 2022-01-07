


class v2(object):
    def __init__(self, x, y):
        self.x = x
        self.y = y

    def zero(self):
        return v2(0,0)

    def inlist(self, list):
        for i in list:
            if i.x == self.x and i.y == self.y:
                return True
        return False
    def inmoveslist(self, list):
        for i in list:
            if i.to.x == self.x and i.to.y == self.y:
                return True
        return False

    def equal(self, pos):
        if pos.x == self.x and pos.y == self.y:
            return True
        return False

alongx = {"h":0,  "g":1,  "f":2,  "e":3,  "d":4,  "c":5,  "b":6,  "a":7}
inverse = {0: "h", 1: "g", 2: "f", 3: "e", 4: "d", 5: "c", 6: "b", 7: "a"}

def getposfromhash(hash):

    letter = hash[0]
    number = int(hash[1])

    return v2(alongx[letter], number - 1)


def gethashfrompos(pos):

    return inverse[pos.x] +str(pos.y + 1)


class piece(object):
    def __init__(self, piecedict, id):
        self.name = piecedict[id]["name"]
        self.colour = piecedict[id]["colour"]
        self.unicode = piecedict[id]["unicode"]
        self.id = id

class move(object):

    def __init__(self, board, to, fro, hash = "", createpiece = None):

        self.additionalmoves = []
        self.to = to
        self.fro = fro
        self.createpiece = createpiece
        if hash != "":
            self.hash = hash
            frohash = hash[0:2]
            tohash = hash[2:4]

            self.fro = getposfromhash(frohash)
            self.to = getposfromhash(tohash)
        else:
            frohash = gethashfrompos(fro)
            tohash = gethashfrompos(to)

            self.hash = frohash + tohash

        self.topiece = board.getpiece(self.to)
        self.fropiece = board.getpiece(self.fro)

        # additional moves needed?
        if board.pawnenpassan and self.topiece.name == "" and self.fropiece.name == "pawn" and self.to.x != self.fro.x:
            colourdirection = {"white":-1, "black":1}
            d = colourdirection[self.fropiece.colour]
            removepos = v2(self.to.x, self.to.y + d)
            self.additionalmoves.append(move(board,removepos,self.to))

        leftrookdic = {"white": v2(0, 0), "black": v2(0, 7)}
        rightrookdic = {"white": v2(7, 0), "black": v2(7, 7)}
        if self.fropiece.name == "king" and self.topiece.name == "" and (self.to.x - self.fro.x) == 2:

            rookfropos = rightrookdic[self.fropiece.colour]
            rooktopos = v2(self.to.x - 1, self.to.y)
            self.additionalmoves.append(move(board,rooktopos, rookfropos))
        elif self.fropiece.name == "king" and self.topiece.name == "" and (self.to.x - self.fro.x) == -2:

            rookfropos = leftrookdic[self.fropiece.colour]
            rooktopos = v2(self.to.x + 1, self.to.y)
            self.additionalmoves.append(move(board, rooktopos, rookfropos))
        # do pawn promotion

        #print(additionalmoves)
        #for mov in additionalmoves:
        #    print(mov.fropiece.name)


    def perform(self, board):
        for move in self.additionalmoves:
            move.additionalmoves = []
            move.perform(board)

        board.overwritepos(self.fro, piece(board.piecedict, "0"))
        if self.createpiece == None:
            board.overwritepos(self.to, self.fropiece)
        else:
            board.overwritepos(self.to, self.createpiece)

    def unperform(self, board):

        board.overwritepos(self.to, self.topiece)
        board.overwritepos(self.fro, self.fropiece)

        for move in self.additionalmoves:
            move.additionalmoves = []
            move.unperform(board)


class board(object):
    def __init__(self, layout, piecedict):
        self.startlayout = layout
        self.history = []
        self.piecedict = piecedict
        self.check = False
        self.pawnenpassan = False

        self.board = self.piecemap(layout)
        self.startboard = self.piecemap(layout)


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
        for mov in self.history:
            if pos.equal(mov.fro) or pos.equal(mov.to):
                return False
        return True

    def gethistoryhash(self):
        return [mov.hash for mov in self.history]

