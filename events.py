# Contains all event classes to be sent through an EventManager

class Event:
    def __init__(self):
        self.name = "Generic Event"
    def __str__(self):
        return "<%s %s>" % (self.__class__.__name__, id(self))

class TickEvent(Event):
    def __init__(self):
        self.name = "Time tick Event"

class QuitEvent(Event):
    def __init__(self):
        self.name = "Program quit Event"

class ClickEvent(Event):
    def __init__(self, position):
        self.name = "Click Event"
        self.pos = position

class UnitMoveRequest(Event):
    def __init__(self, playerID, unitID, position, rotation):
        self.name = "Unit move request Event"
        self.playerID = playerID
        self.unitID = unitID
        self.pos = position
        self.rot = rotation

class UnitPlaceRequest(Event):
    def __init__(self, playerID, unitID, position, rotation):
        self.name = "Unit place request Event"
        self.playerID = playerID
        self.unitID = unitID
        self.pos = position
        self.rot = rotation

class KillEvent(Event):
    def __init__(self, unit, soldier):
        self.name = "Kill Event"
        self.unit = unit
        self.soldier = soldier

class ServerConnectEvent(Event):
    def __init__(self, serverReference):
        self.name = "Network Server Connection Event"
        self.server = serverReference

class ClientConnectEvent(Event):
    def __init__(self, client):
        self.name = "Network Client Connection Event"
        self.client = client

class GameStateRequest(Event):
    # client request for gamestate
    def __init__(self, playerID):
        self.name = "Game State Request Event"
        self.playerID = playerID

class GameStateUpdate(Event):
    # server response to client request for gamestate
    def __init__(self, modeldump):
        self.name = "Game State Update Event"
        self.modeldump = modeldump

class PlayerIDAssignment(Event):
    # server tells client its playerID
    def __init__(self, playerID):
        self.name = "Player ID Assignment Event"
        self.playerID = playerID

class PrivateRemoteEvent(Event):
    # carrier for events thats should only be sent to ONE client, not all
    def __init__(self, event, client):
        self.name = "Private Remote Event"
        self.event = event
        self.client = client

class UnitSelectEvent(Event):
    # client selecting unit to be controlled by the mouse
    def __init__(self, unitID):
        self.name = "Unit Selection Event"
        self.unitID = unitID

class MapSendEvent(Event):
    # server sending map to client at the start of the game
    def __init__(self, mapdump):
        self.name = "Map Send Event"
        self.dump = mapdump

class MapReceivedEvent(Event):
    # client model tells its view that a map is received and it can start the display
    def __init__(self, dimensions):
        self.name = "Map Received Event"
        self.dim = dimensions
