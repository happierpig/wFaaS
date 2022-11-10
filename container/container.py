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
        
    # wait for the container cold start // Q:指的是容器初始化并且启动了Flask？
    def wait_start(self):
        while True:
            try:
                r = requests.get(base_url.format(self.port, 'status'))
                print(r)
                if r.status_code == 200:
                    break
            except Exception:
                pass
            gevent.sleep(0.005)

    # send a request to container and wait for result
    def send_request(self, data = {}):


        r = requests.post(base_url.format(self.port, 'run'), json=data)
        self.lasttime = time.time()

        print(r.json())
        return r.json()

    # initialize the container
    def init(self,function_name):
        data = {'function': function_name, 'concurrency': self.concurrency}
        r = requests.post(base_url.format(self.port, 'init'), json=data)
        self.lasttime = time.time()
        print(r)
        pidList = r.json['pid_list']

        return r.status_code == 200

    # kill and remove the container
    # def cgroup_init(pidList):
        

    def destroy(self):
        self.container.remove(force=True)
