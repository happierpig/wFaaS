from gevent import monkey
monkey.patch_all()
import os
import time
from flask import Flask, request
from gevent.pywsgi import WSGIServer
from wasmtime import Store, Module, Instance
from multiprocessing import Process, Pool



default_file = 'main.wat'
work_dir = '/proxy'

class Runner:
    def __init__(self):
        self.function = None
        self.ctx = {}
        self.runners = None

    def init(self, function, concurrency):
        print('init...')

        # update function status
        self.function = function
        # Init a process pool
        print(concurrency)
        self.runners = Pool(int(concurrency))
        # os.chdir(work_dir)
        pidList = []
        multipleResult = [self.runners.apply_async(os.getpid, ()) for i in range(concurrency)]
        pidList.append(res.get(timeout=1) for res in multipleResult)
        print('init finished..., And the pid list is : ', pidList)
        return pidList

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

#todo
proxy = Flask(__name__)
proxy.status = 'new'
proxy.debug = False
runner = Runner()


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
    pidList = runner.init(inp['function'],inp['concurrency'])

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
    p = Process(target=runner.run)
    p.start()
    p.join()
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
    server = WSGIServer(('127.0.0.1', 24333), proxy)
    server.serve_forever()
