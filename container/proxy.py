from gevent import monkey
monkey.patch_all()
import os
import time
from flask import Flask, request
from gevent.pywsgi import WSGIServer
from gevent.lock import BoundedSemaphore
from wasmtime import Store, Module, Instance
import multiprocessing as mp

default_file = 'main.wat'
work_dir = '/proxy'

# Functions support process pool
pCtx = None # Used to designate the fork manner into 'spawn'

class runnerUnit:
    def __init__(self):
        (self.hostCon, kiteCon) = mp.Pipe()
        self.worker = pCtx.Process(target=occupy_func, args=(kiteCon,))
        self.worker.start() # not use Process.run()
        self.threadLock = BoundedSemaphore(1)
        self.idle = True

    def getvPid(self):
        return self.worker.pid

    def tryOccupy(self):    # Simulate a atomic operation in Thread level
        self.threadLock.acquire()
        flag = self.idle
        if flag == True:
            self.idle = False
        self.threadLock.release()
        return flag == True

    def runCode(self, data): # Must be called after occupy
        self.hostCon.send(data) # wake up the sleepy process
        outputVal = self.hostCon.recv() # block it again
        self.threadLock.acquire()
        self.idle = True
        self.threadLock.release()
        return outputVal
    
    def destroy(self):
        vPid = self.getvPid()
        self.worker.terminate()
        self.worker.close() # Recycle the resources of The Process
        self.hostCon.close()
        return vPid

class runnerPool:
    def __init__(self, _concurrency):
        self.defaultNum = _concurrency
        self.aliveNum = _concurrency
        self.runnerList = [runnerUnit() for i in range(self.defaultNum)]
        self.threadLock = BoundedSemaphore(1)
        return [unit.getvPid() for unit in self.runnerList] # return pidList for Control Group

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
        if flag == True:
            outputVal = candidate.runCode(inputData)
        return flag, outputVal
    
    def addRunner(self):
        newRunner = runnerUnit()
        self.threadLock.acquire()
        self.runnerList.append(newRunner)
        self.aliveNum += 1
        self.threadLock.release()
        return newRunner.getvPid() # For cGroup

    def rmRunner(self):
        # todo : design a remove stragegy 
        # now it always rm the lastest one
        self.threadLock.acquire()
        victimRunner = self.runnerList[self.aliveNum-1]
        vPid = victimRunner.getvPid()
        while(True):
            if victimRunner.tryOccupy() == True:
                break
        victimRunner.destroy()
        self.aliveNum -= 1
        self.runnerList.remove(victimRunner)
        self.threadLock.release()
        return vPid # For cGroup


class Runner:
    def __init__(self):
        self.function = None
        self.ctx = {}
        self.runners = None
        self.concurrency = 4 # Maybe stored in the configuration

    def init(self, function):
        print('init...')
        p = pCtx.Process(target=occupy_func)
        p.start()
        ret = p.pid
        p.join()
        return ret

        # update function status

def occupy_func(con):
    while(True):
        inputData = con.recv() # Blocking for Idle
        # todo : Sent into wasm worker
        outputData = None
        con.send(outputData)

#todo
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
    pidList = runner.init(inp['function'])

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

    end = time.time()

    res = {
        "start_time": start,
        "end_time": end,
        "duration": end - start,
        "inp": inp
    }

    proxy.status = 'ok'

    
    # s = time.time()
    # time.sleep(random.random() * 5)
    # e = time.time()
    # print("Cost time : ",e - s)
    # res = {"res":e-s}
    return res


if __name__ == '__main__':
    pCtx = mp.get_context('spawn')
    print("Init Process Pool Success")
    server = WSGIServer(('0.0.0.0', 23333), proxy)
    server.serve_forever()
