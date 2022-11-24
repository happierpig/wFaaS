import os
import time
from flask import Flask, request
import threading
from wasmtime import Store, Module, Instance
import multiprocessing as mp

default_file = 'main.wat'
work_dir = '/proxy'
pCtx = None # Used to designate the fork manner into 'spawn'
expire_time = 15 # Used to naturally expire the worker's life

# Wrapper Class for Process, maintaining some control states
class runnerUnit:
    def __init__(self):
        (self.hostCon, kiteCon) = mp.Pipe(duplex=True)
        self.worker = pCtx.Process(target=occupy_func, args=(kiteCon,))
        self.worker.start() # not use Process.run()
        self.threadLock = threading.Lock()
        self.idle = True
        self.timeStamp = time.time()

    def getvPid(self):
        return self.worker.pid

    def tryOccupy(self):    # Simulate a atomic operation in Thread level
        self.threadLock.acquire()
        flag = self.idle
        self.idle = False
        self.threadLock.release()
        return flag == True

    def runCode(self, data): # Must be called after occupy
        self.hostCon.send(data) # wake up the sleepy process
        outputVal = self.hostCon.recv() # block it again
        self.threadLock.acquire()
        self.idle = True
        self.timeStamp = time.time()
        self.threadLock.release()
        return outputVal
    
    def checkTime(self): # Must be called after occupy
        self.threadLock.acquire()
        self.idle = True
        self.threadLock.release()
        return (time.time() - self.timeStamp) < expire_time
        
    
    def destroy(self):
        vPid = self.getvPid()
        self.worker.terminate()
        self.worker.close() # Recycle the resources of The Process
        self.hostCon.close()
        return vPid

# Abstraction for managing all-alive process pool
class runnerPool:
    def __init__(self, _concurrency):
        self.defaultNum = _concurrency
        self.aliveNum = _concurrency
        self.runnerList = [runnerUnit() for i in range(self.defaultNum)]
        self.threadLock = threading.Lock()

    def getPids(self):
        return [unit.getvPid() for unit in self.runnerList]
    
    def dispatch_request(self, inputData):
        # todo : some schedule stragegy
        # Now : try all possibility
        flag = False
        outputVal = None
        candidate = None
        self.threadLock.acquire() # avoiding add/remove thread corrupt
        for i in range(self.aliveNum):
            candidate = self.runnerList[i]
            if candidate.tryOccupy() == True:
                flag = True
                break
        self.threadLock.release()
        if flag == False:
            candidate = self.addRunner()
        outputVal = candidate.runCode(inputData)
        return flag, outputVal # flag is false representing that this request call on a new worker process
    
    def addRunner(self):
        newRunner = runnerUnit()
        self.threadLock.acquire()
        self.runnerList.append(newRunner)
        self.aliveNum += 1
        self.threadLock.release()
        return newRunner # Return the object of the new runner for calling it 

    def rmRunner(self):
        # todo : design a remove stragegy 
        # now it always rm the lastest one
        self.threadLock.acquire()
        if self.aliveNum <= self.defaultNum: # Only clean the extra runner
            self.threadLock.release()
            return False
        victimRunner = self.runnerList[self.aliveNum-1]
        if victimRunner.tryOccupy() == False: # Maybe under running
            self.threadLock.release()
            return False # Fail to delete 
        if victimRunner.checkTime() == True: # Maybe still alive
            self.threadLock.release()
            return False
        vPid = victimRunner.destroy()
        self.aliveNum -= 1
        self.runnerList.remove(victimRunner)
        self.threadLock.release()
        return True # Successfully delete

def daemonCleaner(timeSleep, runnersSet): # Function used to clean the expired worker
    while(True):
        time.sleep(timeSleep)
        runnersSet.rmRunner()


# Top Abstraction
class Runner:
    def __init__(self):
        self.function = None
        self.ctx = {}
        self.runners = None
        self.concurrency = None

    def init(self, _function, _concurrency):
        print('init...')
        self.function = _function
        self.concurrency = _concurrency
        self.runners = runnerPool(self.concurrency)
        print('init finish...')
        return self.runners.getPids()

    def run(self, _inputData):
        flag, outputData = self.runners.dispatch_request(_inputData)
        return flag, outputData

def memory_test():
    bigList = []
    count = 1
    while(True):
        bigList.append([0]*4096) # 4k bytes for one time ; 256 times for 1 M 
        time.sleep(0.001)
        if count % 2560 == 0:
            print("Allocate: ",count/2560," M Bytes")
        if count > 2560 * 256:
            break

def occupy_func(con):
    while(True):
        inputData = con.recv() # Blocking for Idle
        # todo : Sent into wasm worker
        outputData = {
            "test": 1,
            "author": 'happypig'
            }
        con.send(outputData)

runner = Runner()
proxy = Flask(__name__)
proxy.status = 'new'
proxy.debug = False


@proxy.route('/status', methods=['GET'])
def status():
    res = {}
    res['status'] = proxy.status
    res['workdir'] = os.getcwd()
    if runner.function:
        res['function'] = runner.function
    return res


@proxy.route('/init', methods=['POST'])
def init():
    proxy.status = 'init'

    inp = request.get_json(force=True, silent=True)
    pidList = runner.init(inp['function'], inp['concurrency'])
    t = threading.Thread(target=daemonCleaner,args=(2, runner.runners,))
    t.start()
    res = {
        "pid_list": pidList
    }
    return res


@proxy.route('/run', methods=['POST'])
def run():
    proxy.status = 'run'

    inp = request.get_json(force=True, silent=True)

    # record the execution time
    start = time.time()
    flag, outputData = runner.run(inp)
    end = time.time()

    res = {
        "start_time": start,
        "end_time": end,
        "duration": end - start,
        "inp": inp,
        "output": outputData,
        "new_worker": flag # False for new worker / True for old worker
    }
    proxy.status = 'ok'
    return res


if __name__ == '__main__':
    # Avoid normal fork() copy thread lock without copying all threads
    pCtx = mp.get_context('spawn')
    print("Init Process Context Successfully and Proxy starts.")
    proxy.run(host='0.0.0.0', port=23333, debug=False, threaded = True)
