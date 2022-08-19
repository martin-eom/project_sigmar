import numpy
import network
from twisted.spread import pb
from twisted.internet import reactor
from twisted.internet.selectreactor import SelectReactor
from twisted.internet.main import installReactor
from twisted.internet.task import LoopingCall
from events import *
from base import EventManager
from serializer import RotToDump
from serializer import OrganizeModelDump as OMoD
from serializer import OrganizeMapDump as OMaD
import pygame

### Contains classes needed for the client as well as main loop of the client.


global serverHost, serverPort
serverHost, serverPort = "localhost", 8000


class ViewEventManager(EventManager):
    # contains some information that the other classes need combined access to
    def __init__(self):
        super().__init__()
        self.playerID = None
        self.selectedUnitID = 0


class NetworkServerView(pb.Root):
    ### Sends objects to server
    STATE_PREP = 0 # preparing
    STATE_CNG = 1 # connecting
    STATE_CNT = 2 # connected
    STATE_DCG = 3 # disconnecting
    STATE_DCT = 4 # disconnected

    def __init__(self, evManager, sharedObjectRegistry):
        self.evManager = evManager
        self.evManager.RegisterListener(self)

        self.pbClientFactory = pb.PBClientFactory()
        self.state = NetworkServerView.STATE_PREP
        self.server = None
        self.sharedObjs = sharedObjectRegistry

    def AttemptConnection(self):
        print("attempting a connection to %s:%s"%(serverHost, serverPort))
        self.state = NetworkServerView.STATE_CNG
        connection = reactor.connectTCP(serverHost, serverPort, self.pbClientFactory)
        deferred = self.pbClientFactory.getRootObject() ### MAKE SURE THE SERVER IS CONNECTED TO THIS FACTORY IN THE MAIN LOOP
        deferred.addCallback(self.Connected)
        deferred.addErrback(self.ConnectFailed)

    def Disconnect(self):
        print("disconnecting")
        print("stopping the reactor")
        reactor.stop()
        self.state = NetworkServerView.STATE_DCG

    def Connected(self, server):
        print("connected")
        self.server = server
        self.state = NetworkServerView.STATE_CNT
        ev = ServerConnectEvent(server)
        self.evManager.Post(ev)

    def ConnectFailed(self, server):
        print("connection failed")
        print(server)
        print("quitting")
        self.evManager.Post(QuitEvent())
        self.sate = NetworkServerView.STATE_DCT

    def Notify(self, event):
        NSV = NetworkServerView
        if isinstance(event, QuitEvent):
            self.Disconnect()
            return

        ev = event
        if not isinstance(event, pb.Copyable):
            evName = event.__class__.__name__
            copyableName = "Copyable" + evName
            if not hasattr(network, copyableName):
                return
            copyableClass = getattr(entwork, copyableName)
            if copyableClass not in network.clientToServerEvents:
                return
            ev = copyableClass(event, self.sharedObjs)
        if ev.__class__ not in network.clientToServerEvents:
            return
        if self.server:
            if not ev.__class__ in self.evManager.noPrint:
                print("Client sending %s"%str(ev))
            remoteCall = self.server.callRemote("EventOverNetwork", ev)
        else:
            print("Can't send while disconnected: %s"%str(ev))


class NetworkServerController(pb.Referenceable):
    ### Receives objects from server
    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)

    def remote_ServerEvent(self, event):
        if not event.__class__ in self.evManager.noPrint:
            print("Received event from server: %s"%str(event))
        self.evManager.Post(event)
        return True

    def Notify(self, event):
        if isinstance(event, ServerConnectEvent):
            event.server.callRemote("ClientConnected", self)


class SimpleGameController:
    # Mousebutton to select location on screen, arrow-keys (l/r) to change unit
    def __init__(self, evManager, model, view):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
        self.model = model
        self.view = view
        self.rot = numpy.eye(2)
        self.p1 = None

    def Notify(self, event):
        from pygame import QUIT, MOUSEBUTTONUP, KEYUP, K_LEFT, K_RIGHT
        from pygame.mouse import get_pos
        if isinstance(event, TickEvent) and not self.view.window is None:
            for event in pygame.event.get([QUIT, MOUSEBUTTONUP, KEYUP]):
                ev = None
                if event.type == pygame.QUIT:
                    ev = QuitEvent()
                elif event.type == MOUSEBUTTONUP:
                    pos = get_pos()
                    ev = ClickEvent([pos[0], -pos[1]])
                elif event.type == KEYUP:
                    if event.key == K_LEFT:
                        ev = UnitSelectEvent((self.evManager.selectedUnitID - 1) % len(self.model\
                            .players[self.evManager.playerID].units))
                    if event.key == K_RIGHT:
                        ev = UnitSelectEvent((self.evManager.selectedUnitID + 1) % len(self.model\
                            .players[self.evManager.playerID].units))
                if ev:
                    self.evManager.Post(ev)
        elif isinstance(event, UnitSelectEvent):
            self.evManager.selectedUnitID = event.unitID
        elif isinstance(event, ClickEvent):
            if not self.rot is None:
                self.p1 = event.pos
                self.rot = None
            else:
                direction = numpy.array(event.pos) - numpy.array(self.p1)
                norm = numpy.linalg.norm(direction)
                sin, cos = direction[1] / norm, direction[0] / norm
                self.rot = numpy.array([[cos, -sin], [sin, cos]])
                if self.model.players[self.evManager.playerID]\
                    .units[self.evManager.selectedUnitID].placed:
                    ev = UnitMoveRequest(self.evManager.playerID, \
                        self.evManager.selectedUnitID, self.p1, RotToDump(self.rot.copy()))
                else:
                    ev = UnitPlaceRequest(self.evManager.playerID, \
                        self.evManager.selectedUnitID, self.p1, RotToDump(self.rot.copy()))
                if ev:
                    self.evManager.Post(ev)


class View:
    import pygame.draw

    def __init__(self, evManager, model):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
        self.window = None
        self.model = model

    def viewPos(pos):
        return (pos[0], -pos[1])

    def NextRender(self):
        from shapes import Arrow
        if not self.window == None:
            self.window.fill((255, 255, 255))
            # green outline and target arrow for selected unit
            if self.model.players and self.model.players[self.evManager.playerID].units:
                unit = self.model.players[self.evManager.playerID]\
                    .units[self.evManager.selectedUnitID]
                if unit.placed:
                    selectorColor = [0, 255, 26]
                    for soldier in unit.soldiers:
                        pygame.draw.circle(self.window, selectorColor, View.viewPos(soldier.pos), unit.rad + 2)
                        #pygame.draw.circle(self.window, selectorColor, View.viewPos(soldier.posTarget), 5) # need to implement transfer of posTarget first
                    pointer = Arrow(unit.posTarget, unit.rotTarget)
                    pygame.draw.polygon(self.window, selectorColor, [View.viewPos(p) for p in pointer.points])
            # blue and green sprites for friendly and enemy units
            for id, player in enumerate(self.model.players):
                if id == self.evManager.playerID:
                    color = [51, 51, 255]
                else:
                    color = [255, 51, 51]
                for unit in player.units:
                    if unit.placed:
                        for soldier in unit.soldiers:
                            pygame.draw.circle(self.window, color, View.viewPos(soldier.pos), unit.rad)
            pygame.display.flip()
    
    def Notify(self, event):
        if isinstance(event, TickEvent):
            self.NextRender()
        elif isinstance(event, MapReceivedEvent):
            self.window = pygame.display.set_mode(event.dim)
            

class ViewModel():

    def __init__(self, evManager):
        self.evManager = evManager
        self.evManager.RegisterListener(self)
        self.players = []
        self.map = ViewMap()

    def Notify(self, event):
        if isinstance(event, GameStateUpdate):
            OMoD(event.modeldump, self)
        elif isinstance(event, PlayerIDAssignment):
            self.evManager.playerID = event.playerID
        elif isinstance(event, MapSendEvent):
            if self.map.dim is None:
                firstTime = True
            else:
                firstTime = False
            OMaD(event.dump, self.map)
            if firstTime:
                ev = MapReceivedEvent(self.map.dim)
                self.evManager.Post(ev)

class ViewPlayer():

    def __init__(self):
        self.units = []

class ViewUnit():

    def __init__(self):
        self.placed = False
        self.rad = None
        self.pos = None
        self.rot = None
        self.posTarget = None
        self.soldiers = []    # soldiers is only a list here

class ViewSoldier():

    def __init__(self):
        self.pos = None

class ViewMap():
    def __init__(self):
        self.dim = None
        
#########################----------------------------------------------------##########################
def main():
    from twisted.internet.task import LoopingCall
    
    fps = 60
    dt = 1 / fps
    gtr = 30 # game tick rate
    gdt = 1 / gtr
    
    evManager = ViewEventManager()
    sharedObjectRegistry = {}

    pygame.init()

    serverController = NetworkServerController(evManager)
    serverView = NetworkServerView(evManager, sharedObjectRegistry)
    
    model = ViewModel(evManager)
    view = View(evManager, model)
    controller = SimpleGameController(evManager, model, view)
    #serverHost = input("Host name: ")
    #serverPort = input("Port: ")
    serverView.AttemptConnection()

    def FireTick(evManager):
        evManager.Post(TickEvent())
    def RequestGameState(evManager):
        evManager.Post(GameStateRequest(evManager.playerID))

    loopingRenderCall = LoopingCall(FireTick, evManager)
    loopingUpdateCall = LoopingCall(RequestGameState, evManager)
    loopingRenderCall.start(dt)
    loopingUpdateCall.start(gdt)
    reactor.run()

if __name__ == "__main__":
    main()
