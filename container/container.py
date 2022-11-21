import requests
import time
import gevent
import os

base_url = 'http://127.0.0.1:{}/{}'
cgroup_path = '/sys/fs/croup/{}/docker/{}/'

class Container:
    # create a new container and return the wrapper (ps: the unit of memory is Mb)
    @classmethod
    def create(cls, client, image_name, port, attr, concurrency = 3, memory = 128):
        container = client.containers.run(image_name,
                                          detach=True,
                                          init=True,
                                          cpuset_cpus='0-3',
                                          cpu_quota=20000,
                                          #pid_mode='host', # For directly get PID in host side
                                          mem_limit='128m',
                                          ports={'23333/tcp': str(port)},
                                          labels=['workflow'])
        res = cls(container, port, attr, concurrency, memory)
        res.wait_start()
        return res

    # get the wrapper of an existed container
    # container_id is the container's docker id
    @classmethod
    def inherit(cls, client, container_id, port, attr):
        container = client.containers.get(container_id)
        return cls(container, port, attr)

    def __init__(self, container, port, attr, concurrency, memory):
        self.container = container
        self.port = port
        self.attr = attr
        self.lasttime = time.time()

        self.concurrency = concurrency
        self.pidList = None # Obtained from docker top
        self.proxyPid = None # Specific for not controling resources 
        self.containerID = None

        self.workerMemoryLimit = memory * 1024 * 1024 # Written in cGroup

    # Tools Function for controling resources
    def get_pids(self):
        raw = self.container.top()['Processes']
        return [unit[1] for unit in raw]
        
    # wait for the container cold start
    def wait_start(self):
        while True:
            try:
                r = requests.get(base_url.format(self.port, 'status'))
                if r.status_code == 200:
                    self.containerID = self.container.id
                    self.proxyPid = self.get_pids()
                    print("Init success and The container ID is : ",self.container.id)
                    break
            except Exception:
                pass
            gevent.sleep(0.005)

    # send a request to container and wait for result
    def send_request(self, data = {}):
        # This is for debug
        data = {
            'time': '2022-10-23'
        }
        r = requests.post(base_url.format(self.port, 'run'), json=data)
        self.lasttime = time.time()
        print(r.json())
        return r.json()

    # initialize the container
    def init(self, function_name, concurrency):
        data = {
            'function': function_name,
            'concurrency': concurrency
            }
        r = requests.post(base_url.format(self.port, 'init'), json=data)
        self.lasttime = time.time()
        # self.pidList = r.json()['pid_list'] # Fake Pids in Container Namespace
        # print(self.pidList)
        self.pidList = self.get_pids()
        time.sleep(3)
        return r.status_code == 200
    
    # Init one limitation on Specific Process
    def add_memoryLimits(self):
        print("Try Modify Cgroup")
        tmpPath = cgroup_path.format('memory',self.containerID) + '/worker'
        if not os.path.exists(tmpPath):
            os.mkdir(tmpPath)
        entries = os.listdir(tmpPath)

        for processID in self.pidList:
            if (processID in self.proxyPid) or (processID in entries):
                continue
            os.mkdir(tmpPath + '/' + str(processID))
            with open(tmpPath + '/' + str(processID) + '/memory.limit_in_bytes','w') as f:
                f.write(self.workerMemoryLimit)
            f.close()
            with open(tmpPath + '/' + str(processID) + '/tasks', 'w') as f:
                f.write(processID)
            f.close()

        with open(tmpPath + '/memory.limit_in_bytes','w') as f:
            f.write(128 * 1024 * 1024 * len(self.proxyPid) + self.workerMemoryLimit * (len(self.pidList)-len(self.proxyPid)))
        f.close()
        print("Modify Cgroup Finish")

    def destroy(self):
        self.container.remove(force=True)
