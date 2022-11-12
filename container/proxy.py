from gevent import monkey
monkey.patch_all()
import os
import time
from flask import Flask, request
from gevent.pywsgi import WSGIServer
from wasmtime import Store, Module, Instance
from concurrent.futures import ProcessPoolExecutor
import random


default_file = 'main.wat'
work_dir = '/proxy'

class Runner:
    def __init__(self):
        self.function = None
        self.ctx = {}
        self.runners = None
        self.concurrency = 4 # Maybe stored in the configuration

    def init(self, function):
        print('init...')

        # update function status
        self.function = function
        pidList = []
        multipleResult = [self.runners.submit(os.getpid,) for i in range(self.concurrency)]
        print("Pid Task dispatched to runners.")
        pidList = [res.result(timeout=3) for res in multipleResult]
        print('init finished..., And the pid list is : ', pidList)
        return pidList

# Concurrent Running Need Non-class function(Maybe for runners in runner)
def run(self):
    self.ctx = {'function': self.function}
    # run function
    store = Store()
    module = Module.from_file(store.engine, default_file)
    instance = Instance(store, module, [])
    func = instance.exports(store)[self.function]
    out = func(store, 27, 6)
    print("This process pid : %d, and result is :%d", os.getpid(),out)
    return out

def run_test():
    sleepTime = random.randint(1,10)
    time.sleep(sleepTime)
    return sleepTime

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
    runnerOut = runner.runners.submit(run_test,)
    retVal = runnerOut.result()
    end = time.time()

    res = {
        "start_time": start,
        "end_time": end,
        "duration": end - start,
        "inp": inp,
        "ret": retVal
    }

    proxy.status = 'ok'

    
    # s = time.time()
    # time.sleep(random.random() * 5)
    # e = time.time()
    # print("Cost time : ",e - s)
    # res = {"res":e-s}
    return res


if __name__ == '__main__':
    print("Proxy Start, and worker concurrecy : ", runner.concurrency)
    runner.runners = ProcessPoolExecutor(max_workers = runner.concurrency)
    print("Init Process Pool Success")
    server = WSGIServer(('0.0.0.0', 23333), proxy)
    server.serve_forever()
