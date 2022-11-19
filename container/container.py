import requests
import time
import gevent

base_url = 'http://127.0.0.1:{}/{}'

class Container:
    # create a new container and return the wrapper
    @classmethod
    def create(cls, client, image_name, port, attr, concurrency = 3):
        container = client.containers.run(image_name,
                                          detach=True,
                                          init=True,
                                          cpuset_cpus='0-3',
                                          cpu_quota=20000,
                                          pid_mode='host', # For directly get PID in host side
                                          mem_limit='128m',
                                          ports={'23333/tcp': str(port)},
                                          labels=['workflow'])
        res = cls(container, port, attr, concurrency)
        res.wait_start()
        return res

    # get the wrapper of an existed container
    # container_id is the container's docker id
    @classmethod
    def inherit(cls, client, container_id, port, attr):
        container = client.containers.get(container_id)
        return cls(container, port, attr)

    def __init__(self, container, port, attr, concurrency):
        self.container = container
        self.port = port
        self.attr = attr
        self.lasttime = time.time()
        self.concurrency = concurrency
        self.pidList = None
        
    # wait for the container cold start
    def wait_start(self):
        while True:
            try:
                r = requests.get(base_url.format(self.port, 'status'))
                if r.status_code == 200:
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
        self.pidList = r.json()['pid_list']
        print(self.pidList)
        time.sleep(3)
        return r.status_code == 200
    
    # Init one limitation on Specific Process
    def add_limit(_pid):
        pass
       

    def destroy(self):
        self.container.remove(force=True)
