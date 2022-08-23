import numpy
from math import ceil

### Contains game physics as well as some mathematical and geometric functions.


def DampenedOscillator(entity, dt):
    allyMaxSpeed = 5*numpy.sqrt(entity.mass)
    allyMaxKnock = 2 / entity.mass
    k = 1
    
    dx = entity.posTarget - entity.pos
    dist = numpy.linalg.norm(dx)
    speed = numpy.linalg.norm(entity.vel)
    velSet = False
    knockSpeed = numpy.linalg.norm(entity.knockVel)
    if not entity.enemyClose:
        if knockSpeed > allyMaxKnock:
            entity.knockVel *= allyMaxKnock / knockSpeed
            knockSpeed = 5
        if entity.allyPassing:
            entity.setTempDamp(allyMaxSpeed)
            #entity.knockVel *= 10
    mu = 20*entity.tempDamp#-0.6
    entity.vel += entity.knockVel
    speed = numpy.linalg.norm(entity.vel)
    if entity.allyPassing and not entity.enemyClose:
        if knockSpeed:
            entity.vel *= allyMaxSpeed/speed
            speed = allyMaxSpeed
            #print("passing")
    if dist < 0.5 and speed < 1 and knockSpeed < 0.5:
        newPos = entity.posTarget
        newVel = numpy.array([0., 0.])
        velSet = True
    else:
        newPos = entity.pos + dt*entity.vel
        #newVel = entity.vel + dt*entity.force
    #if numpy.linalg.norm(newVel) > entity.maxSpeed:
    #    newVel *= entity.maxSpeed/speed
    newForce = k*dx
    strength = numpy.linalg.norm(newForce)
    if strength > entity.Force:
        newForce *= (entity.Force/strength)
        mu = entity.tempDamp
        #if knockSpeed > 0.1*entity.maxSpeed:
        #    mu *= 1 + knockSpeed/entity.maxSpeed
    newForce += mu*entity.vel*speed
    if dist == 0:
        vpar = 0.
    else:
        vpar = numpy.dot(entity.vel, dx)/dist
    if vpar > entity.tempMaxSpeed:
        newForce *= 0
    entity.pos = newPos
    entity.force = newForce
    if not velSet:
        newVel = entity.vel + dt*entity.force
    entity.vel = newVel
    entity.knockVel = numpy.array([0., 0.])
    if entity.allyPassing:
        entity.setTempDamp(entity.maxSpeed)
    entity.allyPassing = False
    entity.enemyClose = False


def KnockUpdate(entity, dt):
    mu = 10
    
    entity.knockForce -= mu*entity.knockVel
    entity.pos += dt*entity.knockVel
    entity.knockVel += dt*entity.knockForce

def posInUnitByIDs(unit):
    nSoldiers = unit.nSoldiers
    nCol = unit.width
    nRows = nSoldiers // nCol + int(nSoldiers%nCol == 1)
    p0 = numpy.array([(nRows-1)/2. * unit.tempSpacing, (nCol-1)/2. * unit.tempSpacing])
    dx, dy = numpy.array([-unit.tempSpacing, 0]), numpy.array([0, -unit.tempSpacing])
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


def RepulsiveForceField(pos1, pos2, rad1, rad2, pID1, pID2, unitID1, unitID2):

    K = 10
    LJ_disp = 2**(1/6) - 1 # relative leonard jones displacement
    minDist = rad1 + rad2
    if pID1 == pID2:
        minDist *= 0.7
    direction = pos2 - pos1
    dist = numpy.linalg.norm(direction)

    def force(dist):    
        force = 0
        #force = numpy.array([0,0,0])
        #minDist *= 0.7
        if pID1 == pID2 or pID1 != pID2:
            #print(dist, minDist)
            if dist < minDist:
                minDist6 = minDist**6
                d2 = dist**2
                d6 = d2**3
                d7 = d6 * dist
                d13 = d7 * d6
                md2 = minDist**2
                md6 = md2**3
                md12 = md6**2
                force += K * (12*md12/d13 - 6*md6/d7)
        return force

    f = force(dist + LJ_disp*minDist)
    #print(f, dist, direction)
    return -force(dist + LJ_disp*minDist) / dist * direction

def CollisionCalculation(entity1, entity2, pID1, pID2, uID1, uID2, dt):
    v1, v2 = numpy.array([0., 0.]), numpy.array([0., 0.])
    minDist = entity1.rad + entity2.rad
    dx = entity2.pos - entity1.pos
    dv = entity2.vel - entity1.vel
    dxdv = numpy.dot(dx, dv)
    movingAway = dxdv > 0
    if pID1 == pID2:
        #minDist *= 0.8
        if uID1 != uID2 and not movingAway:
            entity1.allyPassing = True
            entity2.allyPassing = True
            #entity1.unit.allyClose = True
            #entity2.unit.allyClose = True
    else:
        #minDist *= 1.1
        entity1.enemyClose = True
        entity2.enemyClose = True
        entity1.unit.EnemyClose()
        entity2.unit.EnemyClose()
    if pID1 == pID2:
        if uID1 != uID2 and not entity1.unit.enemyClose and not entity2.unit.enemyClose:
            minDist *= 0.5
            entity1.unit.AllyPassing()
            entity2.unit.AllyPassing()
        else:
            minDist *= 0.8
    else:
        minDist *= 1.1
    dist = numpy.linalg.norm(dx)
    if dist > minDist:
        return v1, v2
    if movingAway:    # checks if they are moving away from each other (post-collision)
        return v1, v2
    M = entity1.mass + entity2.mass
    prod = 2.*dxdv*dx/(M*dist**2)
    v1 = entity2.mass*prod
    v2 = -entity1.mass*prod
    return v1, v2
