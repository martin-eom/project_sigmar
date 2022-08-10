# Makes it so that events can be sent over the network and sorts them by sending direction

from events import *
from twisted .spread import pb

serverToClientEvents = []
clientToServerEvents = []

def MixInClass(Class, addClass):
    if addClass not in Class.__bases__:
        Class.__bases__ += (addClass),
def MixInCopyClasses(Class):
    MixInClass(Class, pb.Copyable)
    MixInClass(Class, pb.RemoteCopy)


# Tick Event
# is internal to the model and therefore the server.

# Quit Event
# created on servr but should be sent to client.
# Direction: ServerToClient
MixInCopyClasses(QuitEvent)
pb.setUnjellyableForClass(QuitEvent, QuitEvent)
clientToServerEvents.append(QuitEvent)

# ClickEvent
# is internal to the clients.

# UnitMoveRequest
# Direction: ClientToServer
MixInCopyClasses(UnitMoveRequest)
pb.setUnjellyableForClass(UnitMoveRequest, UnitMoveRequest)
clientToServerEvents.append(UnitMoveRequest)

# UnitPlaceRequest
# Direction: ClientToServer
MixInCopyClasses(UnitPlaceRequest)
pb.setUnjellyableForClass(UnitPlaceRequest, UnitPlaceRequest)
clientToServerEvents.append(UnitPlaceRequest)

# KillEvent
# is internal to the model and therefore the server.

# ServerConnectEvent
# is internal to client.

# ClientConnectedEvent
# is internal to server.

# GameStateRequest
# Direction: ClientToServer
MixInCopyClasses(GameStateRequest)
pb.setUnjellyableForClass(GameStateRequest, GameStateRequest)
clientToServerEvents.append(GameStateRequest)

# GameStateUpdate
# Direction: ServerToClient
MixInCopyClasses(GameStateUpdate)
pb.setUnjellyableForClass(GameStateUpdate, GameStateUpdate)
serverToClientEvents.append(GameStateUpdate)

# PlayerIDAssignment
# Direction: ServerToClient
MixInCopyClasses(PlayerIDAssignment)
pb.setUnjellyableForClass(PlayerIDAssignment, PlayerIDAssignment)
serverToClientEvents.append(PlayerIDAssignment)

# Private Remote Event
# is internal to server

# UnitSelectEvent
# is internal to the client.

# MapSendEvent
# Direction: ServerToClient
MixInCopyClasses(MapSendEvent)
pb.setUnjellyableForClass(MapSendEvent, MapSendEvent)
serverToClientEvents.append(MapSendEvent)

# MapReceivedEvent
# is internal to client.
