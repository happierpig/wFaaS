from functionInfo import FunctionInfo
from functionSet import RequestInfo
from container import Container
from gevent.lock import BoundedSemaphore
import gevent

clean_interval = 5 # TODO

# functionUnit class abstract single User's Single Function
# Embedded Session Scheduler
class FunctionUnit:
    def __init__(self, dockerClient, portMan, userID, funcInfo):
        self.userID = userID
        self.funcInfo = funcInfo
        self.dockerClient = dockerClient
        self.portMan = portMan

        self.lock = BoundedSemaphore(1)
        self.workerCount = 0
        self.workers = []
        self.maxTaskLimits = []
        self.nowTasks = []

        self.mainIp = None

        # todo : 1. check portMan's thread-safe 2. add necessary info into function info.
    
    # Function for initializing the wasm processes
    def init(self):
        self.workers.append(Container.create(self.dockerClient, self.funcInfo.img_name, self.portMan.get(), 'exec', True))
        self.workers[0].init(self.funcInfo.function_name)
        self.mainIp = self.get_main_ip()
        self.maxTaskLimits.append(10)
        self.nowTasks.append(0)
        self.workerCount = 1
        
        gevent.spawn_later(clean_interval, self.daemon_cleaner)

    def get_main_ip(self):
        return self.dockerClient.api.inspect_container(self.workers[0].containerID)['NetworkSettings']['IPAddress']


    def send_request(self, requestInfo):
        candidateWorker = None
        self.lock.acquire()
        for i in range(0, self.workerCount):
            if self.nowTasks[i] < self.maxTaskLimits[i]:
                candidateWorker = i
                break

        if candidateWorker == None: # All container full and we need add one 
            if self.workerCount == 0:
                self.workers.insert(self.workerCount, Container.create(self.dockerClient, self.funcInfo.img_name,
                        self.portMan.get(), 'exec', True))
                self.mainIp = self.get_main_ip()
            else:
                self.workers.insert(self.workerCount, Container.create(self.dockerClient, self.funcInfo.img_name, 
                        self.portMan.get(), 'exec', False, self.mainIp))
            self.maxTaskLimits.insert(self.workerCount, 10)
            self.nowTasks.insert(self.workerCount, 0)
            self.workers[self.workerCount].init(self.funcInfo.function_name)
            candidateWorker = self.workerCount
            self.workerCount += 1
            
        self.nowTasks[candidateWorker] += 1
        self.lock.release()

        ret = self.workers[candidateWorker].send_request(requestInfo.data)

        self.lock.acquire()
        self.nowTasks[candidateWorker] -= 1
        self.lock.release()
        
        requestInfo.result.set(ret)

    def container_cleaner(self):
        self.lock.acquire()
        candidateWorker = self.workerCount - 1
        if (candidateWorker < 0) or (self.nowTasks[candidateWorker] > 0) :
            self.lock.release()
            return
        if self.workers[candidateWorker].get_status() != "idle":
            self.lock.release()
            return
        self.portMan.put(self.workers[candidateWorker].port)
        self.workers[candidateWorker].destroy()
        self.workers.pop(candidateWorker)
        self.workerCount -= 1
        self.lock.release()
    
    def daemon_cleaner(self):
        self.container_cleaner()
        gevent.spawn_later(clean_interval, self.daemon_cleaner)
