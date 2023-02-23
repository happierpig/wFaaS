from functionInfo import FunctionInfo
from functionUnit import FunctionUnit
from functionUnit import RequestInfo

# FunctionSet represents all functionUnit with same function
class FunctionSet:
    def __init__(self, client, funcInfo, port_controller):
        self.dockerClient = client
        self.funcInfo = funcInfo # All subunits in this class belongs to the same function infomation
        self.portMan = port_controller
        self.userWorker = {} # Hashmap for mapping userid to functionUnit
        
        self.requestQueue = []

    # blocking function ; need async like gevent above for concurrency
    def accept_request(self, user_id, request_id, runtime, input, output, to, keys):
        data = {'request_id': request_id, 'runtime': runtime, 'input': input, 'output': output, 'to': to, 'keys': keys}
        rq = RequestInfo(request_id, data, user_id)
        self.requestQueue.append(rq)
        retVal = rq.result.get()
        return retVal

    def dispatch_request(self):
        if len(self.requestQueue) == 0:
            return
        rq = self.requestQueue.pop(0)
        theWorker = self.userWorker.get(key=rq.userID, default=None)
        if theWorker == None:   # this user first appears 
            # todo: precreate User Unit / Check thread safety
            # create Unit
            theWorker = FunctionUnit(self.dockerClient, self.portMan, rq.userID, self.funcInfo)
            theWorker.init()
            self.userWorker[rq.userID] = theWorker
        theWorker.send_request(rq)

