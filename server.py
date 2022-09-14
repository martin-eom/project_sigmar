#! /usr/bin/env python

from base import EventManager
from serializer import ModelToDump, MapToDump
from serializer import OrganizeRotDump as ORD
import units
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
        self.time = time()
        # performance measurement
        self.tick_time = 0.
        self.oscillator_time = 0.
        self.map_grid_time = 0.
        self.resolution_time = 0.
        self.clean_time = 0.
        self.physics_time = 0.
        self.t0 = time()

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
        def step(i, map_row, j, tile, sid1, sold1):
            #for sid1, sold1 in enumerate(tile):
            #    pool.apply_async(step, [i, map_row, j, tile, sid1, sold1])
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
        
        results = []
        
        for i, map_row in enumerate(self.map.grid):
            for j, tile in enumerate(map_row):
                for sid1, sold1 in enumerate(tile):
                    step(i, map_row, j, tile, sid1, sold1)
                    """for sid2, sold2 in enumerate(tile[(sid1 + 1):len(tile)]):
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
                            sold2[0].knockVel += v2"""

    def DetectEnemies(self):
        for player in self.players:
            for unit in player.units:
                if unit.enemyClose:
                    enemyClose = False
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
                if unit.allyClose:
                    allyClose = False
                    for soldier in list(chain(*unit.soldiers)):
                        if (not soldier is None) and soldier.allyClose:
                            allyClose = True
                            break
                    if not allyClose:
                        print("Ally gone.")
                        #unit.ChangeSpacing(unit.spacing)
                        #unit.spacingChange = False
                        #unit.tempSpacing = unit.spacing
                        #unit.MoveTarget(unit.posTarget, unit.rotTarget)
                        #unit.spacingTicks = 0
                    unit.allyClose = allyClose
                        

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
            t_tick_0 = time()
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
                t = time()
                print("Game speed: %.2f tps"%(self.tps/(t - self.time)))
                print("Usage of tick time per function:")
                t_cur = time() - self.t0
                #print("Dampened Oscillator:     %.2f"%(self.oscillator_time / self.tick_time))
                #print("Making map grid:         %.2f"%(self.map_grid_time / self.tick_time))
                print("Physics:                 %.2f"%(self.physics_time / self.tick_time))
                print("Collision Resolution:    %.2f"%(self.resolution_time / self.tick_time))
                print("Cleaning map grid:       %.2f"%(self.clean_time / self.tick_time))
                print("Usage of total time on TickEvent: %.2f"%(self.tick_time / t_cur))
                self.time = t
            if self.tickNum % (5*self.tps) == 0:
                self.tick_time = 0.
                self.oscialltor_time = 0.
                self.map_grid_time = 0.
                self.resolution_time = 0.
                self.clean_time = 0.
                self.physics_time = 0.
                self.t0 = time()

            self.SpacingChange()
            results = []
            def physics_step(soldier, pID, uID):
                if soldier is not None and soldier.pos is not None:
                    physics.DampenedOscillator2(soldier, self.dt)
                    soldier.gridpos = self.map.Assign(soldier, pID, uID)
            t_0 = time()
            for pID, player in enumerate(self.players):
                for uID, unit in enumerate(player.units):
                    if unit.placed:
                        # soldierIDs?
                        for rID, row in enumerate(unit.soldiers):
                            for cID, soldier in enumerate(row):
                                physics_step(soldier, pID, uID)
                                """if soldier is not None and soldier.pos is not None:
                                    t_0 = time()
                                    physics.DampenedOscillator2(soldier, self.dt)
                                    self.oscillator_time += time() - t_0
                                    t_0 = time()
                                    soldier.gridpos = self.map.Assign(soldier, pID, uID)
                                    self.map_grid_time += time() - t_0
                                    # iterate displacement from special force"""
                        unit.updatePos()
            self.physics_time += time() - t_0
            # combat
            t_0 = time()
            self.CollisionResolution()
            self.resolution_time += time() - t_0
            #for player in self.players:
            #    for unit in player.units:
            #        for row in unit.soldiers:
            #            for soldier in row:
            #                if soldier is not None and soldier.pos is not None:
            #                    KnockUpdate(soldier, self.dt)
            t_0 = time()
            self.map.cleangrid()
            self.clean_time += time() - t_0
            self.tick_time += time() - t_tick_0
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
        self.units = [units.SimpleUnit(), units.SimpleUnit(), \
                      units.MonsterUnit(), units.CavalryUnit()]
        
    def addUnit(self, unit):
        if unit not in self.units:
            self.units.append(unit)
        else:
            print("[WARNING:] Tried to add unit to player twice.")
        

def countNestedListNotNone(soldiers):
    return len([1 for soldier in list(chain(*soldiers)) if not soldier is None])


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
