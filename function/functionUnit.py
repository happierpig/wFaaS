from functionInfo import FunctionInfo
from functionSet import RequestInfo
from container import Container

# functionUnit class abstract single User's Single Function
class FunctionUnit:
    def __init__(self, dockerClient, portMan, userID, funcInfo):
        self.userID = userID
        self.funcInfo = funcInfo
        self.dockerClient = dockerClient
        self.portMan = portMan
        self.worker = Container.create(dockerClient, self.funcInfo.img_name, self.portMan.get(), 'exec')
        self.worker.init(self.funcInfo.function_name)
        # todo : 1. check portMan's thread-safe 2. add necessary info into function info.
    
    def send_request(self, requestInfo):
        ret = self.worker.send_request(requestInfo.data)
        requestInfo.result.set(ret)
