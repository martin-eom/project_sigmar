from itertools import chain
from numpy import array

# Serialzation functions for transfer via network

def ModelToDump(model):
    dump = []
    for player in model.players:
        dump.append("[")
        for unit in player.units:
            dump.append("[")
            dump.append(unit.placed)
            if unit.placed:
                for tupel in (unit.pos, unit.posTarget, unit.rotTarget[0], unit.rotTarget[1]):
                    for number in tupel:
                        dump.append(float(number))
                for soldier in list(chain(*unit.soldiers)):
                    if not soldier is None:
                        for number in soldier.pos:
                            dump.append(float(number))
            dump.append("]")
        dump.append("]")
    return dump

def OrganizeModelDump(dump, simplemodel):
    from client import ViewModel, ViewPlayer, ViewUnit, ViewSoldier
    depth = 0
    nplayers = 0
    while dump:
        symbol = dump.pop(0)
        if symbol == "[":
            if depth == 0:
                nplayers += 1
                players = simplemodel.players
                if nplayers > len(players):
                    players.append(ViewPlayer())
                player = players[nplayers - 1]
                units = player.units
                nunits = 0
            elif depth == 1:
                nunits += 1
                if nunits > len(units):
                    units.append(ViewUnit())
                unit = units[nunits - 1]
                unit.placed = dump.pop(0)
                if unit.placed:
                    nsoldiers = 0
                    unit.pos = array([dump.pop(0), dump.pop(0)])
                    unit.posTarget = array([dump.pop(0), dump.pop(0)])
                    unit.rotTarget = array([[dump.pop(0), dump.pop(0)], [dump.pop(0), dump.pop(0)]])
                    soldiers = unit.soldiers
            depth += 1
        elif symbol == "]":
            if depth == 2:
                if unit.placed:
                    List, lim = soldiers, nsoldiers
                else:
                    List, lim = [], 0
            elif depth == 1:
                List, lim = units, nunits
            else:
                List, lim = players, nplayers
            spill = len(List) - lim
            for i in range(spill):
                List.pop(-1)
            depth -= 1
        else:
            nsoldiers += 1
            if nsoldiers > len(soldiers):
                soldiers.append(ViewSoldier())
            soldier = soldiers[nsoldiers - 1]
            soldier.pos = array([symbol, dump.pop(0)])

def MapToDump(gamemap):
    return gamemap.dim

def OrganizeMapDump(dump, gamemap):
    gamemap.dim = dump
        
def RotToDump(rot):
    return [float(rot[0,0]), float(rot[0,1]), float(rot[1,0]), float(rot[1,1])]
        
def OrganizeRotDump(dump):
    a, b, c, d = dump.pop(0), dump.pop(0), dump.pop(0), dump.pop(0)
    return array([[a, b],[c, d]])
