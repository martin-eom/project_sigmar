from events import *
from itertools import chain
import numpy
from physics import posInUnitByIDs, turnArray, angleDirection, Reform

### Contains class for Event Manager, Soldier, Unit and some basic functions


class EventManager:
    
    def __init__(self):
        from weakref import WeakKeyDictionary
        self.noPrint = [TickEvent, GameStateRequest, GameStateUpdate, PrivateRemoteEvent]
        self.listeners = WeakKeyDictionary()
        self.eventQueue = []
        self.fps = 30
        self.dt = 1 / self.fps

    def RegisterListener(self, listener):
        self.listeners[listener] = True

    def UnregisterListener(self, listener):
        if listener in self.listeners:
            del self.listeners[listener]

    def Post(self, event):
        if not event.__class__ in self.noPrint:
            print(" - Event Posted: " + event.name)
        for listener in self.listeners:
            listener.Notify(event)

    def ConsumeEventQueue(self):
        i = 0
        while i < len(self.eventQueue):
            event = self.eventQueue[i]
            for listener in self.listeners:
                listener.Notify(event)
            i += 1
        self.eventQueue = []


def countNestedListNotNone(soldiers):
    return len([1 for soldier in list(chain(*soldiers)) if not soldier is None])


class Soldier:
    rad = 5
    mass = 1.
    defaultMaxSpeed = 15.
    accel = 3.
    turn = numpy.pi / 1.5   # turnspeed

    def __init__(self, unit = None):
        self.Force = self.mass*self.accel
        self.defaultDamp = -self.Force/(self.defaultMaxSpeed**2)
        self.pos = None
        self.posTarget = self.pos
        self.oldPosTarget = self.posTarget
        self.rot = None
        self.rotTarget = None
        self.vel = numpy.array([0.,0.])
        self.knockVel = numpy.array([0.,0.])
        self.force = numpy.array([0.,0.])
        self.gridpos = None
        self.allyClose = False
        self.enemyClose = False
        self.maxSpeed = self.defaultMaxSpeed
        self.Damp = self.defaultDamp
        self.unit = unit

    def setDamp(self, speed):
        self.maxSpeed = speed
        self.Damp = -self.Force/(self.maxSpeed**2)


class Unit:
    width = 1
    maxSoldiers = 1
    spacing = 0
    wideSpacing = 0
    soldierClass = Soldier
    
    def __init__(self):
        self.placed = False
        self.pos = None
        self.rot = None
        self.posTarget = None
        self.rotTarget = None
        self.vel = None
        self.force = None
        self.tempSpacing = self.spacing
        self.allyClose = False
        self.enemyClose = False
        self.spacingTick = 0
        self.spacingChange = False

        self.soldiers = []
        for i in range(self.maxSoldiers//self.width + int(self.maxSoldiers%self.width != 0)):
            self.soldiers.append([])
            for j in range(self.width):
                self.soldiers[i].append(None)
        for i in range(self.maxSoldiers):
            self.soldiers[i//self.width][i%self.width] = self.soldierClass(self)
        self.nSoldiers = countNestedListNotNone(self.soldiers)
        self.posInUnit = posInUnitByIDs(self)
    
    def Place(self, position, rotation):
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].pos = position + numpy.dot(rotation, self.posInUnit[i][j])
                    self.soldiers[i][j].posTarget = self.soldiers[i][j].pos
                    self.soldiers[i][j].rot = rotation
                    self.soldiers[i][j].rotTarget = rotation
                    self.soldiers[i][j].vel = numpy.array([0., 0.])
                    self.soldiers[i][j].force = numpy.array([0., 0.])
        self.pos = position
        self.posTarget = position
        self.rot = rotation
        self.rotTarget = rotation
        self.placed = True

    def MoveTarget(self, position, rotation):
        self.posTarget = position
        self.rotTarget = rotation
        self.ReformForTurn()
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].posTarget = position + numpy.dot(rotation, self.posInUnit[i][j])
                    self.soldiers[i][j].rotTarget = rotation

    def updatePos(self):
        if self.soldiers:
            self.pos = sum(soldier.pos for soldier in list(chain(*self.soldiers)) if soldier is not None) / self.nSoldiers
        else:
            self.pos = numpy.array([0,0])

    def ReformForTurn(self):
    # reassigns the soldiers to different array positions to minimize chaos when turning more than 45 degrees
        rot = numpy.dot(self.rotTarget, numpy.linalg.inv(self.rot))
        self.soldiers = turnArray(angleDirection(rot), self.soldiers)
        self.rot = self.rotTarget
        self.soldiers = Reform(self.soldiers, self.width)
        self.posInUnit = posInUnitByIDs(self)

    def ReformAfterLoss(self):

    ###########!!!!!!########## # Only works for turning and kills, but not when units are scattered after a charge attack.
                                # Proposed solution: Use a different reorganization structure which is triggered maybe once a second.
                                # Unassign all soldiers from their position in unit.
                                # while there are unassigned soldiers:
                                #   Find the soldier furthest unassigned away from the unit posiiton target.
                                #   Assign it to the closest (to itself) position in unit at target that is still unassigned.
        
    # resassigns the soldiers to different array positions to fill holes in the front line
        if len(self.soldiers) > 1:
            for i, deadSol in enumerate(self.soldiers[0]):
                if deadSol is None:
                    potentialReplacements = list(enumerate(self.soldiers[1][:i+1]))
                    potentialReplacements.reverse()
                    for j, soldier in potentialReplacements:
                        if not soldier is None:
                            self.soldiers[0][i] = soldier
                            self.soldiers[1][j] = None
                            break
                    break
            rowEmpty = True
            for soldier in self.soldiers[1]:
                if not soldier is None:
                    rowEmpty = False
                    break
            if rowEmpty:
                self.soldiers.pop(1)
        print(self.soldiers)
        self.soldiers = Reform(self.soldiers, self.width)
        currentSize = countNestedListNotNone(self.soldiers)
        self.nSoldiers = currentSize
        self.posInUnit = posInUnitByIDs(self)
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if not self.soldiers[i][j] is None:
                    self.soldiers[i][j].posTarget = self.posTarget + numpy.dot(self.rotTarget, self.posInUnit[i][j])
