import requests
import time
import gevent
import os

base_url = 'http://127.0.0.1:{}/{}'
cgroup_path = '/sys/fs/cgroup/{}/docker/{}'

class Container:
    # create a new container and return the wrapper (ps: the unit of memory is Mb)
    @classmethod
    def create(cls, client, image_name, port, attr, concurrency = 3, memory = 128, cpuRes = 0.2):
        container = client.containers.run(image_name,
                                          detach=True,
                                          init=True,
                                          cpuset_cpus='0-3',
                                          cpu_quota=20000,
                                          #pid_mode='host', # For directly get PID in host side
                                          mem_limit='128m',
                                          ports={'23333/tcp': str(port)},
                                          labels=['wFaaS'])
        res = cls(container, port, attr, concurrency, memory, cpuRes)
        res.wait_start()
        return res

    # get the wrapper of an existed container
    # container_id is the container's docker id
    @classmethod
    def inherit(cls, client, container_id, port, attr):
        container = client.containers.get(container_id)
        return cls(container, port, attr)

    def __init__(self, container, port, attr, concurrency, memory, cpuRes):
        self.container = container
        self.port = port
        self.attr = attr
        self.lasttime = time.time()

        self.concurrency = concurrency
        self.pidList = None # Obtained from docker top
        self.proxyPid = None # Specific for not controling resources 
        self.containerID = None

        self.workerMemoryLimit = memory * 1024 * 1024 # Written in cGroup
        self.cpuLimit = (int)(cpuRes * 100000) # Written in cGroup

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
        resp = r.json()
        if(resp['new_worker'] == False): # New worker exists
            self.add_cputLimits()
            self.add_memoryLimits()
        print(resp)
        return resp

    # initialize the container
    def init(self, function_name):
        data = {
            'function': function_name,
            'concurrency': self.concurrency
            }
        r = requests.post(base_url.format(self.port, 'init'), json=data)
        self.lasttime = time.time()
        # self.pidList = r.json()['pid_list'] # Fake Pids in Container Namespace
        # print(self.pidList)
        self.pidList = self.get_pids()
        for tmpEntry in self.proxyPid:
            if not tmpEntry in self.pidList:
                self.proxyPid.remove(tmpEntry)
        # time.sleep(3) # Avoid process not ready for assignments
        self.add_memoryLimits()
        self.add_cputLimits()
        return r.status_code == 200
    
    # Init one limitation on Specific Process
    def add_memoryLimits(self):
        tmpPath = cgroup_path.format('memory',self.containerID) + '/worker'
        if not os.path.exists(tmpPath):
            os.mkdir(tmpPath)
        entries = os.listdir(tmpPath)

        for processID in self.pidList:
            if (processID in self.proxyPid) or (processID in entries):
                continue
            os.mkdir(tmpPath + '/' + str(processID))
            with open(tmpPath + '/' + str(processID) + '/memory.limit_in_bytes','w') as f:
                f.write(str(self.workerMemoryLimit))
            f.close()
            with open(tmpPath + '/' + str(processID) + '/tasks', 'w') as f:
                f.write(str(processID))
            f.close()

        with open(tmpPath + '/memory.limit_in_bytes','w') as f:
            f.write(str(self.workerMemoryLimit * (len(self.pidList)-len(self.proxyPid))))
        f.close()

        with open(cgroup_path.format('memory',self.containerID) + '/memory.limit_in_bytes','w') as f:
            f.write(str(128*1024*1024*len(self.proxyPid) + self.workerMemoryLimit * (len(self.pidList)-len(self.proxyPid))))
        f.close()        
    
    def add_cputLimits(self):
        tmpPath = cgroup_path.format('cpu,cpuacct',self.containerID) + '/worker'
        if not os.path.exists(tmpPath):
            os.mkdir(tmpPath)
        entries = os.listdir(tmpPath)

        for processID in self.pidList:
            if processID in entries:
                continue
            os.mkdir(tmpPath + '/' + str(processID))
            with open(tmpPath + '/' + str(processID) + '/cpu.cfs_quota_us','w') as f:
                f.write(str(self.cpuLimit))
            f.close()
            with open(tmpPath + '/' + str(processID) + '/tasks', 'w') as f:
                f.write(str(processID))
            f.close()

        with open(cgroup_path.format('cpu,cpuacct',self.containerID) + '/cpu.cfs_quota_us','w') as f:
            f.write(str(self.cpuLimit * len(self.pidList)))
        f.close()     

        with open(tmpPath + '/cpu.cfs_quota_us','w') as f:
            f.write(str(self.cpuLimit * len(self.pidList)))
        f.close()   
        
    def limits_recycle(self):
        print("Try Recycle the cgroup subsystem")
        tmpPath = cgroup_path.format('cpu,cpuacct',self.containerID) + '/worker'
        entries = os.listdir(tmpPath)
        for processID in entries:
            if processID in self.pidList:
                continue
        # todo: delete the useless directory
        pass

    def destroy(self):
        self.container.remove(force=True)