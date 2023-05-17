import requests
import time
import gevent
import os
from docker.types import Mount

base_url = 'http://127.0.0.1:{}/{}'
cgroup_path = '/sys/fs/cgroup/system.slice/docker-{}.scope'

class Container:
    # create a new container and return the wrapper (ps: the unit of memory is Mb)
    @classmethod
    def create(cls, client, image_name, port, attr, isMain, isOss, codeMntPath, mainIp = "", mainPort = 0, concurrency = 3, memory = 128, cpuRes = 0.2):
        # We constraint the container's cpu and memory resource by artificially modify cgroup file
        container = client.containers.run(image_name,
                                          detach=True,
                                          init=True,
                                        #   cpuset_cpus='0-3',
                                        #   cpu_quota=200000,
                                          #pid_mode='host', # For directly get PID in host side
                                        #   mem_limit='128m',
                                          ports={'18000/tcp': str(port)},
                                          labels=['wFaaS'],
                                          mounts=[
                                            Mount(target='/code', source=codeMntPath, type='bind', read_only=True)
                                          ]
                                        )
        res = cls(container, port, attr, isMain, isOss, mainIp, mainPort, concurrency, memory, cpuRes)
        res.wait_start()
        return res

    # get the wrapper of an existed container
    # container_id is the container's docker id
    @classmethod
    def inherit(cls, client, container_id, port, attr):
        container = client.containers.get(container_id)
        return cls(container, port, attr)

    def __init__(self, container, port, attr, isMain, isOss, mainIp, mainPort, concurrency, memory, cpuRes):
        self.container = container
        self.port = port
        self.attr = attr
        self.isMain = isMain
        self.isOss = isOss
        self.mainIp = mainIp
        self.mainPort = mainPort
        self.lasttime = time.time()

        self.concurrency = concurrency
        self.pidList = [] # Including for all processes in the container
        self.proxyPid = [] # Specific for proxy process with 128m/1cpu
        self.containerID = None

        self.workerMemoryLimit = memory * 1024 * 1024 # Written in cGroup / megaBytes
        self.cpuLimit = (int)(cpuRes * 100000) # Propotional to PERIOD in cgroup cpu.max

    # Tools Function for controling resources
    def get_pids(self):
        raw = self.container.top()['Processes']
        return [unit[1] for unit in raw]
        
    def get_status(self):
        return ((requests.get(base_url.format(self.port, 'status'))).json())["status"]

    # wait for the container cold start
    def wait_start(self):
        while True:
            try:
                r = requests.get(base_url.format(self.port, 'status'))
                if r.status_code == 200:
                    self.containerID = self.container.id
                    self.proxyPid = self.get_pids()
                    # Init cgroup configuration
                    topCgroupPath = cgroup_path.format(self.containerID)
                    tmpPath = topCgroupPath + '/xfaas'
                    if not os.path.exists(tmpPath):
                        os.mkdir(tmpPath)
                    for processID in self.proxyPid:
                        os.mkdir(tmpPath + '/' + str(processID))
                        with open(tmpPath + '/' + str(processID) + '/cgroup.procs', 'w') as f:
                            f.write(str(processID))
                    with open(topCgroupPath + '/cgroup.subtree_control','w') as f:
                        f.write("+cpu +memory")
                    with open(tmpPath + '/cgroup.subtree_control','w') as f:
                        f.write("+cpu +memory")
                    for processID in self.proxyPid:
                        with open(tmpPath + '/' + str(processID) + '/memory.max','w') as f:
                            f.write(str(256 * 1024 * 1024))
                        with open(tmpPath + '/' + str(processID) + '/cpu.max','w') as f:
                            f.write(str(100000))
                    print(f"Init success and The container ID is: {self.container.id}, Proxy Pids: {self.proxyPid}")
                    break
            except Exception:
                pass
            gevent.sleep(0.005)

    # send a request to container and wait for result
    def send_request(self, data = {}):
        # This is for debug
        data["echo"] = -1 # -1 means first-pass
        r = requests.post(base_url.format(self.port, 'run'), json=data)
        self.lasttime = time.time()
        resp = r.json()
        if(resp['new_worker'] == False):
            self.pidList = self.get_pids()
            self.recycle_limits()
            self.add_limits()
            data["echo"] = resp["echo"]
            r = requests.post(base_url.format(self.port, 'run'), json=data)
            resp = r.json()
            assert(resp['new_worker'] == True)
            resp['new_worker'] = False
        return resp

    # initialize the container
    def init(self, function_name):
        data = {
            'function': function_name,
            'concurrency': self.concurrency,
            'id': self.containerID,
            'isMain': self.isMain,
            'ip': self.mainIp,
            'mainPort': self.mainPort,
            'isOss': self.isOss
        }
        r = requests.post(base_url.format(self.port, 'init'), json=data)
        self.lasttime = time.time()
        self.pidList = self.get_pids()
        for tmpEntry in self.proxyPid:
            if not tmpEntry in self.pidList:
                self.proxyPid.remove(tmpEntry)
        self.recycle_limits()
        self.add_limits()
        return r.status_code == 200
    
    # Init one limitation on Specific Process
    def add_limits(self):
        tmpPath = cgroup_path.format(self.containerID) + '/xfaas'
        if not os.path.exists(tmpPath):
            os.mkdir(tmpPath)
        entries = os.listdir(tmpPath)

        for processID in self.pidList:
            if (processID in entries):
                continue
            os.mkdir(tmpPath + '/' + str(processID))
            with open(tmpPath + '/' + str(processID) + '/memory.max','w') as f:
                if processID in self.proxyPid:
                    f.write(str(256 * 1024 * 1024))
                else:
                    f.write(str(self.workerMemoryLimit))
            with open(tmpPath + '/' + str(processID) + '/cpu.max','w') as f:
                if processID in self.proxyPid:
                    f.write(str(100000))
                else: 
                    f.write(str(self.cpuLimit))
            with open(tmpPath + '/' + str(processID) + '/cgroup.procs', 'w') as f:
                f.write(str(processID))
        
    def recycle_limits(self):
        tmpPath = cgroup_path.format(self.containerID) + '/xfaas'
        entries = os.listdir(tmpPath)
        for processID in entries:
            checkDirPath = tmpPath + '/' + str(processID)
            if os.path.isdir(checkDirPath):
                if len(open(checkDirPath + '/cgroup.procs', 'r').readlines()) == 0:
                    os.rmdir(checkDirPath)

    def adapt_proxy_cpu(self, containerSize):
        newCpuRes = (int) (100000 * (1 + 0.5 * (containerSize - 1)))
        newMemoryRes = (int) (128 * 1024 * 1024 * containerSize)
        tmpPath = cgroup_path.format(self.containerID) + '/xfaas'
        for processID in self.proxyPid:
            with open(tmpPath + '/' + str(processID) + '/cpu.max','w') as f:
                f.write(str(newCpuRes))
            with open(tmpPath + '/' + str(processID) + '/memory.max','w') as f:
                f.write(str(newMemoryRes))

    def destroy(self):
        self.container.remove(force=True)