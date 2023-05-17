from functionInfo import FunctionInfo
from container import Container
from gevent.lock import BoundedSemaphore
from gevent import event
import gevent
import time

clean_interval = 5 # TODO

class RequestInfo:
    def __init__(self, requestID, data, userID):
        self.userID = userID
        self.requestID = requestID
        self.data = data
        self.result = event.AsyncResult()
        self.arrival = time.time()

# functionUnit class abstract single User's Single Function
# Embedded Session Scheduler
class FunctionUnit:
    def __init__(self, dockerClient, portMan, userID, funcInfo, mntPath, isOss=False, parrelWorker=10):
        self.userID = userID
        self.funcInfo = funcInfo
        self.dockerClient = dockerClient
        self.portMan = portMan

        self.lock = BoundedSemaphore(1)
        self.shortLock = BoundedSemaphore(1) # This is for addContainer
        self.scaleFlag = False

        self.workerCount = 0
        self.workers = []
        self.maxTaskLimits = []
        self.nowTasks = []

        self.mainIp = None
        self.mainPort = None
        self.isOss = isOss

        self.codeMntPath = mntPath

        self.parrelWorker = parrelWorker
    
    # Function for initializing the wasm processes
    def init(self):
        self.workers.append(Container.create(self.dockerClient, self.funcInfo.img_name, self.portMan.get(), 
                                             'exec', True, isOss=self.isOss, codeMntPath=self.codeMntPath))
        self.workers[0].init(self.funcInfo.function_name)
        self.mainIp = self.get_main_ip()
        self.mainPort = self.get_main_port()
        self.maxTaskLimits.append(self.parrelWorker)
        self.nowTasks.append(0)
        self.workerCount = 1
        self.maxWorker = self.funcInfo.max_containers
        
        # Comment this for not occupying the resources
        # gevent.spawn_later(clean_interval, self.daemon_cleaner)

    def get_main_ip(self):
        return self.dockerClient.api.inspect_container(self.workers[0].containerID)['NetworkSettings']['IPAddress']

    def get_main_port(self):
        return self.workers[0].port

    def horizontal_scale(self):
        self.shortLock.acquire()
        tmpFlag = self.scaleFlag
        self.scaleFlag = True
        self.shortLock.release()

        if tmpFlag == True: # Only executed once
            return None
        
        candidateWorker = None
        # For now, since no daemon cleaner and no other adder, the self.workerCount is fixed. We do not need lock
        if self.workerCount < self.maxWorker:
            if self.workerCount == 0:
                self.workers.insert(self.workerCount, Container.create(self.dockerClient, self.funcInfo.img_name,
                        self.portMan.get(), 'exec', True, isOss=self.isOss, codeMntPath=self.codeMntPath))
                self.mainIp = self.get_main_ip()
                self.mainPort = self.get_main_port()
            else:
                start = time.time()
                self.workers.insert(self.workerCount, Container.create(self.dockerClient, self.funcInfo.img_name, 
                    self.portMan.get(), 'exec', False, isOss=self.isOss, codeMntPath=self.codeMntPath, mainIp=self.mainIp, mainPort=self.mainPort))
                end = time.time()
                print("[Session] Creating Container use ", end - start)
            self.workers[self.workerCount].init(self.funcInfo.function_name) # Concurrency security guaranteed by executed-once
            candidateWorker = self.workerCount

            self.lock.acquire()
            self.maxTaskLimits.insert(self.workerCount, self.parrelWorker)
            self.nowTasks.insert(self.workerCount, 1) # Directly assigned one task
            self.workerCount += 1
            self.lock.release()

            # Fuel more resource into proxy in main container.
            self.workers[0].adapt_proxy_cpu(self.workerCount)
            print("[SessionScheduler] Add ", self.workerCount, "th Container")


        self.shortLock.acquire()
        self.scaleFlag = False
        self.shortLock.release()
        return candidateWorker

    def send_request(self, requestInfo):
        startTime = time.time_ns()
        candidateWorker = None
        self.lock.acquire()
        while candidateWorker == None:
            for i in range(0, self.workerCount):
                if self.nowTasks[i] < self.maxTaskLimits[i]:
                    candidateWorker = i
                    break
            if candidateWorker != None:
                self.nowTasks[candidateWorker] += 1
                self.lock.release()
                break
            else:
                self.lock.release()
                candidateWorker = self.horizontal_scale()
                if candidateWorker == None:
                    gevent.sleep(0.001)
                    self.lock.acquire()

        ret = self.workers[candidateWorker].send_request(requestInfo.data)

        self.lock.acquire()
        self.nowTasks[candidateWorker] -= 1
        self.lock.release()

        print("[SessionScheduler] Finish ", requestInfo.data['request_id'])
        endTime = time.time_ns()
        ret["twoPartTime"] = (endTime - startTime) / 1000
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
        print("[SessionScheduler] Remove", candidateWorker+1, "th Container")
        self.lock.release()
    
    def daemon_cleaner(self):
        self.container_cleaner()
        gevent.spawn_later(clean_interval, self.daemon_cleaner)

    def destroy(self):
        for i in range(0, self.workerCount):
            self.workers[i].destroy()
        self.workerCount = 0
        self.workers = []
        self.maxTaskLimits = []
        self.nowTasks = []
        print("[SessionScheduler] Destroyed")
