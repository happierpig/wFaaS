import gevent
import docker
import os
from functionInfo import parse
from portController import PortController
from functionSet import FunctionSet

dispatch_interval = 0.005 # 200 qps at most

# functionManager schedules the functions daemon operations and global objects
class functionManager:
    def __init__(self, configPath, minPort):
        self.functionInfos = parse(configPath)
        self.portMan = PortController(minPort, minPort+4999)
        self.dockerClient = docker.from_env()

        self.functions = {
            x.function_name: FunctionSet(self.dockerClient, x, self.portMan)
            for x in self.functionInfos
        }

    def init(self):
        print("Clearing previous containers.")
        os.system('docker rm -f $(docker ps -aq --filter label=wFaaS)')
        gevent.spawn_later(dispatch_interval, self._dispatch_loop)

    # todo: What does spawn_later do?
    def _dispatch_loop(self):
        gevent.spawn_later(dispatch_interval, self._dispatch_loop)
        for function in self.functions.values():
            gevent.spawn(function.dispatch_request)

    # blocking function ; need async call for concurrency
    def run(self, function_name, userID, request_id, runtime, input, output, to, keys):
        # print('run', function_name, request_id, runtime, input, output, to, keys)
        if function_name not in self.functions:
            raise Exception("No such function!")
        return self.functions[function_name].accept_request(userID, request_id, runtime, input, output, to, keys)