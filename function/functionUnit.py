from functionInfo import FunctionInfo
from functionSet import RequestInfo
from container import Container
import os
import socket

socket_address = '/tmp/{}_{}/pipe'

# functionUnit class abstract single User's Single Function
class FunctionUnit:
    def __init__(self, dockerClient, portMan, userID, funcInfo):
        self.userID = userID
        self.funcInfo = funcInfo
        self.dockerClient = dockerClient
        self.portMan = portMan
        self.pipeAddress = socket_address.format(funcInfo.function_name, userID)
        self.pipe = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        if os.path.exists(self.pipeAddress):
            os.unlink(self.pipeAddress)
        self.pipe.bind(self.pipeAddress)
        self.pipe.listen(5)

        # todo : 1. check portMan's thread-safe 2. add necessary info into function info.
    
    # Function for initializing the wasm processes ; todo : add data structure
    def init(self):
        self.worker = Container.create(self.dockerClient, self.funcInfo.img_name, self.portMan.get(), 'exec')
        self.worker.init(self.funcInfo.function_name)

    def send_request(self, requestInfo):
        ret = self.worker.send_request(requestInfo.data)
        requestInfo.result.set(ret)
    
    # Should be called by thread
    def daemon_cleaner(self):
        while(True):
            con, addr = self.pipe.accept()
            victimId = con.recv(1024) # The container ID to be deleted
            # todo : select the one in the data structure
            con.close()
            self.worker.destroy()
            self.worker = None

