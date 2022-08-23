#! /usr/bin/env python

from base import EventManager
from serializer import ModelToDump, MapToDump
from serializer import OrganizeRotDump as ORD
import numpy
from events import *
import network
from time import time
from twisted.spread import pb
from math import ceil
import _pickle as pickle
def deepcopy(a):
    return pickle.loads(pickle.dumps(a, -1))

### Contains classes needed for the server as well as the main loop of the server.


class NetworkClientController(pb.Root):
    # Receives messages from the clients
    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)

    def remote_ClientConnected(self, client):
        print("remote client connected")
        print("client:")
        print(client)
        ev = ClientConnectEvent(client)
        self.evManager.Post(ev)
        return True

    def remote_GetObjectState(self, objectID):
        if not sharedObjectRegistry.has_key(objectID):
            print("No key in server")
            return [0, 0]
        obj = sharedObjectRegistry[objectID]
        objDict = obj.getStateToCopy(sharedObjectRegistry)
        return [objectID, objDict]

    def remote_EventOverNetwork(self, event):
        self.evManager.Post(event)
        return True

    def Notify(self, event):
        pass


class NetworkClientView(pb.Referenceable):
    # Sends messages to the clients
    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
        self.clients = []

    def EventThatShouldBeSentToAll(self, event):
        if isinstance(event, ClientConnectEvent):
            self.clients.append(event.client)

        ev = event
        # We don't have events requiring this yet, but for the future:
        if not isinstance(event, pb.Copyable):
            evName = event.__class__.__name__
            copyableClassName = "Copyable" + evName
            if not hasattr(network, copyableClassName):
                return None
            copyableClass = getattr(network, copyableClassName)
            ev = copyableClass(ev, sharedObjectRegistry)
        if ev.__class__ not in network.serverToClientEvents:
            #if ev.__class__ not in self.evManager.noPrint:
            #    print("SERVER NOT SENDING: " + str(ev))
            return None

        return ev

    def Notify(self, event):
        if isinstance(event, PrivateRemoteEvent):
            ev = event.event
            if not ev.__class__ in self.evManager.noPrint:
                print(" - Sending event %s to %s"%(str(ev), event.client))
            remoteCall = event.client.callRemote("ServerEvent", ev)
        else:
            ev = self.EventThatShouldBeSentToAll(event)
            if not ev:
                return
            for client in self.clients:
                if not ev.__class__ in self.evManager.noPrint:
                    print("\n - Sending event: %s to client %d"%(str(ev), self.clients.index(client)))
                remoteCall = client.callRemote("ServerEvent", ev)


class Game:
    # contains game state, player references?
    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
    
    def Notify(self, event):
        pass


class Map:
    gridsize = 31

    # container for solid objects and some form of neighbourlisting
    def __init__(self, dimensions):
        self.dim = dimensions
        self.griddim = (ceil(self.dim[1] / Map.gridsize), ceil(self.dim[0] / Map.gridsize))
        self.cleangrid()

    def cleangrid(self):
        self.grid = []
        for i in range(self.griddim[0]):
            self.grid.append([])
            for j in range(self.griddim[1]):
                self.grid[-1].append([])

    def Assign(self, soldier, playerID, unitID):
        gridpos = (int(soldier.pos[0] / Map.gridsize), int(soldier.pos[1] / Map.gridsize))
        self.grid[gridpos[0]][gridpos[1]].append((soldier, playerID, unitID))
        return gridpos


from numpy import array
import physics
#from physics import DampenedOscillator, KnockUpdate, RepulsiveForceField
from itertools import chain

class Model:

    def __init__(self, evManager, Map):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
        self.players = []
        self.clientsByPlayerID = {}
        self.map = Map
        self.dt = evManager.dt
        self.dump = []
        self.tOfLastDump = time()
        self.tickNum = 0
        self.tps = int(ceil(1 / self.dt)) # ticks per second

    def NeighbourForces(self):
        for i, map_row in enumerate(self.map.grid):
            for j, tile in enumerate(map_row):
                for sid1, sold1 in enumerate(tile):
                    for sid2, sold2 in enumerate(tile[sid1 + 1 : len(tile)]):
                        f = physics.RepulsiveForceField(sold1[0].pos, sold2[0].pos, sold1[0].rad, sold2[0].rad, \
                                                sold1[1], sold2[1], sold1[2], sold2[2])
                        sold1[0].force += f
                        sold2[0].force -= f
                        #sold1[0].knockForce = +f
                        #sold2[0].knockForce = -f
                    for neighbourIDs in (((i+1)%self.map.griddim[0], j), \
                                        (i, (j+1)%self.map.griddim[1]), \
                                        ((i+1)%self.map.griddim[0], \
                                        (j+1)%self.map.griddim[1])):
                        tile = self.map.grid[neighbourIDs[0]][neighbourIDs[1]]
                        for sid2, sold2 in enumerate(tile):
                            f = physics.RepulsiveForceField(sold1[0].pos, sold2[0].pos, sold1[0].rad, sold2[0].rad, \
                                                    sold1[1], sold2[1], sold1[2], sold2[2])
                            sold1[0].force += f
                            sold2[0].force -= f
                            #sold1[0].knockForce = +f
                            #sold2[0].knockForce = -f

    def CollisionResolution(self):
        for i, map_row in enumerate(self.map.grid):
            for j, tile in enumerate(map_row):
                for sid1, sold1 in enumerate(tile):
                    for sid2, sold2 in enumerate(tile[(sid1 + 1):len(tile)]):
                        v1, v2 = physics.CollisionCalculation(sold1[0], sold2[0], sold1[1], sold2[1], sold1[2], sold2[2], self.dt)
                        sold1[0].knockVel += v1
                        sold2[0].knockVel += v2
                    for neighbourIDs in (((i+1)%self.map.griddim[0], j), \
                                        (i, (j+1)%self.map.griddim[1]), \
                                        ((i+1)%self.map.griddim[0], \
                                        (j+1)%self.map.griddim[1])):
                        tile = self.map.grid[neighbourIDs[0]][neighbourIDs[1]]
                        for sid2, sold2 in enumerate(tile):
                            v1, v2 = physics.CollisionCalculation(sold1[0], sold2[0], sold1[1], sold2[1], sold1[2], sold2[2], self.dt)
                            sold1[0].knockVel += v1
                            sold2[0].knockVel += v2

    def DetectEnemies(self):
        for player in self.players:
            for unit in player.units:
                if unit.enemyClose:
                    enemyClose = False
                    #unit.enemyClose = False
                    for soldier in list(chain(*unit.soldiers)):
                        if not soldier is None and soldier.enemyClose:
                            enemyClose = True
                            break
                    if not enemyClose:
                        print("Enemy gone.")
                    unit.enemyClose = enemyClose

    def DetectAllies(self):
        for player in self.players:
            for unit in player.units:
                if unit.allyPassing:
                    allyPassing = False
                    #unit.allyPassing = False
                    for soldier in list(chain(*unit.soldiers)):
                        if (not soldier is None) and soldier.allyPassing:
                            allyPassing = True
                            break
                    if not allyPassing:
                        print("Ally gone.")
                        unit.ChangeSpacing(unit.spacing)
                        #unit.spacingChange = False
                        #unit.tempSpacing = unit.spacing
                        #unit.MoveTarget(unit.posTarget, unit.rotTarget)
                        #unit.spacingTicks = 0
                    unit.allyPassing = allyPassing
                        

    def SpacingChange(self):
        for player in self.players:
            for unit in player.units:
                if unit.spacingChange:
                    if unit.spacingTicks < 2*self.tps:
                        for soldier in list(chain(*unit.soldiers)):
                            if not soldier is None and not soldier.pos is None:
                                step = 1/(2*self.tps) * (soldier.posTarget - soldier.oldPosTarget)
                                soldier.pos += step
                        unit.spacingTicks += 1
                    else:
                        unit.spacingChange = False
                        unit.spacingTicks = 0
                        print("Finished changing spacing.")
        
    def Notify(self, event):
        ev = None
        if isinstance(event, ClientConnectEvent):
            playerID = len(self.clientsByPlayerID)
            ev = PlayerIDAssignment(playerID)
            self.evManager.Post(PrivateRemoteEvent(ev, event.client))
            ev = MapSendEvent(MapToDump(self.map))
            self.evManager.Post(PrivateRemoteEvent(ev, event.client))
            self.clientsByPlayerID[playerID] = event.client
            self.players.append(Player(self.evManager))
            print("Client %s has been asigned playerID %d"%(event.client, playerID))
        elif isinstance(event, UnitPlaceRequest):
            playerID, unitID = event.playerID, event.unitID
            if -1 < playerID < len(self.players) and -1 < unitID < len(self.players[playerID].units):
                rot = ORD(event.rot)
                self.players[playerID].units[unitID].Place(event.pos, rot)
            else:
                print("[WARNING:] Tried to place unit that was not in player unit list.")
        elif isinstance(event, UnitMoveRequest):
            playerID, unitID = event.playerID, event.unitID
            if -1 < playerID < len(self.players) and -1 < unitID < len(self.players[playerID].units):
                rot = ORD(event.rot)
                self.players[playerID].units[unitID].MoveTarget(event.pos, rot)
                ### The following segment is for debugging purposes only. Kills random number of soldiers in the front row after every MoveRequest.
                """
                from random import randint
                unit = self.players[event.playerID].units[event.unitID]
                for row in unit.soldiers:
                    dead = {}
                    for soldier in row:
                        dead[soldier] = randint(0,1)
                    for soldier in dead.keys():
                        if dead[soldier]:
                            evManager.Post(KillEvent(unit, soldier))
                        #if not randint(0,1) and event.unit.nSoldiers > 1:
                        #    evManager.Post(KillEvent(event.unit, row, soldier))
                """
            else:
                print("[WARNING:] Tried to move unit that was not in player unit list.")
        elif isinstance(event, TickEvent):
            self.tickNum += 1
            """for pID, player in enumerate(self.players):
                for uID, unit in enumerate(player.units):
                    if unit.placed:
                        # soldierIDs?
                        for rID, row in enumerate(unit.soldiers):
                            for cID, soldier in enumerate(row):
                                if soldier is not None and soldier.pos is not None:
                                    soldier.gridpos = self.map.Assign(soldier.pos, pID, uID, rID, cID, rad)
            """
            if self.tickNum % self.tps == 0:
                self.DetectEnemies()
                self.DetectAllies()
            self.SpacingChange()
            for pID, player in enumerate(self.players):
                for uID, unit in enumerate(player.units):
                    if unit.placed:
                        # soldierIDs?
                        for rID, row in enumerate(unit.soldiers):
                            for cID, soldier in enumerate(row):
                                if soldier is not None and soldier.pos is not None:                                    
                                    physics.DampenedOscillator(soldier, self.dt)
                                    soldier.gridpos = self.map.Assign(soldier, pID, uID)
                                    # iterate displacement from special force
                        unit.updatePos()
            # combat
            self.CollisionResolution()
            #for player in self.players:
            #    for unit in player.units:
            #        for row in unit.soldiers:
            #            for soldier in row:
            #                if soldier is not None and soldier.pos is not None:
            #                    KnockUpdate(soldier, self.dt)
            self.map.cleangrid()
        elif isinstance(event, KillEvent):
            soldierFound = False
            for row in event.unit.soldiers:
                for i, soldier in enumerate(row):
                    if soldier == event.soldier:
                        row[i] = None
                        soldierFound = True
                        break
                if soldierFound:
                    event.unit.nSoldiers -= 1
                    break
            if event.unit.nSoldiers == 0:
                event.unit.soldiers = []
                #event.unit.placed = False
            else:
                event.unit.ReformAfterLoss()
        elif isinstance(event, GameStateRequest):
            if time() - self.tOfLastDump > self.dt:
                self.dump = ModelToDump(self)
            ev = GameStateUpdate(self.dump)
            self.evManager.Post(PrivateRemoteEvent(ev, self.clientsByPlayerID[event.playerID]))

class Player:

    def __init__(self, evManager):
        self.units = [SimpleUnit(), SimpleUnit(), MonsterUnit(), CavalryUnit()]
        
    def addUnit(self, unit):
        if unit not in self.units:
            self.units.append(unit)
        else:
            print("[WARNING:] Tried to add unit to player twice.")
        

from physics import posInUnitByIDs, Reform, angleDirection, turnArray, removeNone, paddNone


class Soldier:
    rad = 5
    mass = 1.
    maxSpeed = 15.
    accel = 3.

    def __init__(self, unit = None):
        self.Force = self.mass*self.accel
        self.Damp = -self.Force/(self.maxSpeed**2)
        self.pos = None
        self.posTarget = self.pos
        self.oldPosTarget = self.posTarget
        self.vel = array([0.,0.])
        self.knockVel = array([0.,0.])
        self.force = array([0.,0.])
        self.gridpos = None
        self.allyPassing = False
        self.enemyClose = False
        self.tempMaxSpeed = self.maxSpeed
        self.tempDamp = self.Damp
        self.unit = unit

    def setTempDamp(self, speed):
        self.tempMaxSpeed = speed
        self.tempDamp = -self.Force/(self.tempMaxSpeed**2)

class Monster(Soldier):
    rad = 15
    mass = 9.
    maxSpeed = 100.
    accel = 4.

    def __init__(self, unit = None):
        super().__init__(unit)

class Rider(Soldier):
    rad = 7
    mass = 3.
    maxSpeed = 40.
    accel = 3.

    def __init__(self, unit = None):
        super().__init__(unit)
                                
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
        self.allyPassing = False
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
            #self.soldiers[i//self.width][i%self.width].unit = self
        self.nSoldiers = countNestedListNotNone(self.soldiers)
        self.posInUnit = posInUnitByIDs(self)
    
    """def Place(self, position, rotation):
        self.pos = array(position)
        self.rot = rotation
        self.posTarget = position
        self.rotTarget = rotation
        self.vel = array([0, 0])
        self.force = array([0, 0])"""

    def Place(self, position, rotation):
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].pos = position + numpy.dot(rotation, self.posInUnit[i][j])
                    self.soldiers[i][j].posTarget = self.soldiers[i][j].pos
                    self.soldiers[i][j].vel = array([0., 0.])
                    self.soldiers[i][j].force = array([0., 0.])
        self.pos = position
        self.posTarget = position
        self.rot = rotation
        self.rotTarget = rotation
        self.placed = True
        
    """def MoveTarget(self, position, rotation):
        self.posTarget = array(position)
        self.rotTarget = rotation"""

    def MoveTarget(self, position, rotation):
        self.posTarget = position
        self.rotTarget = rotation
        self.ReformForTurn()
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].posTarget = position + numpy.dot(rotation, self.posInUnit[i][j])

    def updatePos(self):
        if self.soldiers:
            self.pos = sum(soldier.pos for soldier in list(chain(*self.soldiers)) if soldier is not None) / self.nSoldiers
        else:
            self.pos = array([0,0])

    def adjustSize(self):
        currentSize = countNestedListNotNone(self.soldiers)
        print("current size: %d; previous size: %d; diff: %d"%(currentSize,self.nSoldiers,self.nSoldiers-currentSize))
        if not currentSize == self.nSoldiers:
            print("########### %d soldiers have been killed! ###########"%(self.nSoldiers-currentSize))
            self.nSoldiers = currentSize
            self.soldiers = Reform(self.soldiers, self.width)
            self.posInUnit = posInUnitByIDs(self)
            for i in range(len(self.soldiers)):
                for j in range(len(self.soldiers[i])):
                    self.soldiers[i][j].posTarget = self.posTarget + numpy.dot(self.rotTarget, self.posInUnit[i][j])

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

    def EnemyClose(self):
        if not self.enemyClose:
            print("Enemy close.")
            self.enemyClose = True
            self.ChangeSpacing(self.spacing)
            #self.tempSpacing = self.spacing
            #self.MoveTarget(self.posTarget, self.rotTarget)
            #self.spacingTicks = 0
            #self.spacingChange = True

    def AllyPassing(self):
        if not self.allyPassing:
            print("Ally close.")
            self.allyPassing = True
            self.ChangeSpacing(self.wideSpacing)
            #self.tempSpacing = self.wideSpacing
            #self.MoveTarget(self.posTarget, self.rotTarget)
            #self.spacingTicks = 0
            #self.spacingChange = True
        ##### we need to force them to change their spacing quickly!!!!!!#

    def ChangeSpacing(self, spacing):
        if not self.spacingChange:
            for soldier in list(chain(*self.soldiers)):
                soldier.oldPosTarget = soldier.posTarget.copy()
            self.tempSpacing = spacing
            self.MoveTarget(self.posTarget, self.rotTarget)
            self.spacingTicks = 0
            self.spacingChange = True
            print("Changing unit spacing.")


def countNestedListNotNone(soldiers):
    return len([1 for soldier in list(chain(*soldiers)) if not soldier is None])


class MonsterUnit(Unit):
    width = 1
    maxSoldiers = 1
    spacing = 0
    wideSpacing = 0
    soldierClass = Monster

    def __init__(self):
        super().__init__()

class CavalryUnit(Unit):
    width = 4
    maxSoldiers = 16
    spacing = 18
    wideSpacing = 20
    soldierClass = Rider

class SimpleUnit(Unit):
    width = 8
    maxSoldiers = 64
    spacing = 12
    wideSpacing = 15
    soldierClass = Soldier
    
    def __init__(self):
        super().__init__()
        """self.soldiers = []
        for i in range(self.maxSoldiers//self.width + int(self.maxSoldiers%self.width != 0)):
            self.soldiers.append([])
            for j in range(self.width):
                self.soldiers[i].append(None)
        for i in range(self.maxSoldiers):
            self.soldiers[i//self.width][i%self.width] = self.soldierClass()
        self.nSoldiers = countNestedListNotNone(self.soldiers)
        self.posInUnit = posInUnitByIDs(self)"""
        
    """def Place(self, position, rotation):
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].pos = position + numpy.dot(rotation, self.posInUnit[i][j])
                    self.soldiers[i][j].posTarget = self.soldiers[i][j].pos
                    self.soldiers[i][j].vel = array([0, 0])
                    self.soldiers[i][j].force = array([0, 0])
        self.pos = position
        self.posTarget = position
        self.rot = rotation
        self.rotTarget = rotation
        self.placed = True"""
        
    """def MoveTarget(self, position, rotation):
        self.posTarget = position
        self.rotTarget = rotation
        self.ReformForTurn()
        for i in range(len(self.soldiers)):
            for j in range(len(self.soldiers[i])):
                if self.soldiers[i][j] is not None:
                    self.soldiers[i][j].posTarget = position + numpy.dot(rotation, self.posInUnit[i][j])"""
        
    """def updatePos(self):
        if self.soldiers:
            self.pos = sum(soldier.pos for soldier in list(chain(*self.soldiers)) if soldier is not None) / self.nSoldiers
        else:
            self.pos = array([0,0])"""

    """def adjustSize(self):
        currentSize = countNestedListNotNone(self.soldiers)
        print("current size: %d; previous size: %d; diff: %d"%(currentSize,self.nSoldiers,self.nSoldiers-currentSize))
        if not currentSize == self.nSoldiers:
            print("########### %d soldiers have been killed! ###########"%(self.nSoldiers-currentSize))
            self.nSoldiers = currentSize
            self.soldiers = Reform(self.soldiers, self.width)
            self.posInUnit = posInUnitByIDs(self)
            for i in range(len(self.soldiers)):
                for j in range(len(self.soldiers[i])):
                    self.soldiers[i][j].posTarget = self.posTarget + numpy.dot(self.rotTarget, self.posInUnit[i][j])"""
        
    """def ReformForTurn(self):
    # reassigns the soldiers to different array positions to minimize chaos when turning more than 45 degrees
        rot = numpy.dot(self.rotTarget, numpy.linalg.inv(self.rot))
        self.soldiers = turnArray(angleDirection(rot), self.soldiers)
        self.rot = self.rotTarget
        self.soldiers = Reform(self.soldiers, self.width)
        self.posInUnit = posInUnitByIDs(self)"""
        
    """def ReformAfterLoss(self):
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
                    self.soldiers[i][j].posTarget = self.posTarget + numpy.dot(self.rotTarget, self.posInUnit[i][j])"""


class ConsoleView:

    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)

    def Notify(self, event):
        if not isinstance(event, TickEvent):
            print(event.name)


evManager = None
sharedObjectRegistry = None

######################----------------------------------------------------------#######################
def main():
    from twisted.internet.task import LoopingCall
    from twisted.internet import reactor

    def FireTick(evManager):
        evManager.Post(TickEvent())

    global evManager, sharedObjectRegistry
    evManager = EventManager()
    sharedObjectRegistry = {}

    gameMap = Map([1000, 1000])
    model = Model(evManager, gameMap)
    clientController = NetworkClientController(evManager)
    clientView = NetworkClientView(evManager)
    
    loopingCall = LoopingCall(FireTick, evManager)
    ticks_per_second = 30
    dt = 1 / ticks_per_second
    loopingCall.start(dt)

    reactor.listenTCP(8000, pb.PBServerFactory(clientController))
    print("listening for clients...")
    reactor.run()

if __name__ == "__main__":
    print("starting server...")
    main()
