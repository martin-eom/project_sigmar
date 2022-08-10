import numpy
from math import ceil

def DampenedOscillator(entity, dt):
    k = 1e-1
    mu = 0.6
    
    newPos = entity.pos + dt*entity.vel
    newVel = entity.vel + dt*entity.force
    speed = numpy.linalg.norm(newVel)
    if numpy.linalg.norm(newVel) > entity.maxSpeed:
        newVel *= entity.maxSpeed/speed
    newForce = k*(entity.posTarget - entity.pos) - mu*entity.vel
    entity.pos = newPos
    entity.vel = newVel
    entity.force = newForce


def posInUnitByIDs(unit):
    nSoldiers = unit.nSoldiers
    nCol = unit.width
    nRows = nSoldiers // nCol + int(nSoldiers%nCol == 1)
    p0 = numpy.array([(nRows-1)/2. * unit.spacing, (nCol-1)/2. * unit.spacing])
    dx, dy = numpy.array([-unit.spacing, 0]), numpy.array([0, -unit.spacing])
    posInUnit = []
    for i in range(len(unit.soldiers)):
        posInUnit.append([])
        for j in range(len(unit.soldiers[i])):
            posInUnit[i].append(p0 + i * dx + j * dy)
    return posInUnit


def Reform(oldSoldiers, width):
    removeNone(oldSoldiers, width)
    midpoint = width / 2
    insertLoc = ceil(midpoint)
    newSoldiers = []
    oldRow = oldSoldiers.pop(0)
    newRow = []
    while oldSoldiers or oldRow:    # empty lists are False
        while not oldRow:
            oldRow = oldSoldiers.pop(0)
        if len(newRow) < midpoint:
            newRow.append(oldRow.pop(0))
        else:
            newRow.insert(insertLoc, oldRow.pop(-1))
        if len(newRow) == width:
            newSoldiers.append(newRow)
            newRow = []
    if newRow:
        newSoldiers.append(newRow)
    paddNone(newSoldiers, width)
    return newSoldiers

    
def angleDirection(rotation):
    ang = numpy.arctan2(rotation[1][0], rotation[0][0])
    if -numpy.pi/4 <= ang <= numpy.pi/4:
        return "Front"
    elif numpy.pi/4 < ang < 3*numpy.pi/4:
        return "Left"
    elif -3*numpy.pi/4 < ang < -numpy.pi/4:
        return "Right"
    else:
        return "Back"
        

def turnArray(direction, soldiers):
    if direction == "Back":
        for row in soldiers:
            row.reverse()
        soldiers.reverse()
        return soldiers
    elif direction == "Left":
        newSoldiers = []
        for i in range(max(len(row) for row in soldiers)):
            newSoldiers.append([])
            for row in soldiers:
                if row:
                    newSoldiers[-1].insert(0, row.pop(0))
                else:
                    soldiers.remove(row)
        return newSoldiers
    elif direction == "Right":
        newSoldiers = []
        for i in range(max(len(row) for row in soldiers)):
            newSoldiers.insert(0,[])
            for row in soldiers:
                if row:
                    newSoldiers[0].append(row.pop(0))
                else:
                    soldiers.remove(row)
        return newSoldiers
    else:
        return soldiers


def removeNone(soldiers, width):
    for i, row in enumerate(soldiers):
        soldiers[i] = [soldier for soldier in soldiers[i] if not soldier is None]
        
def paddNone(soldiers, width):
    while len(soldiers[-1]) < width:
        soldiers[-1].append(None)
