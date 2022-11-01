import requests
import time
import gevent
from gevent.lock import BoundedSemaphore

base_url = 'http://127.0.0.1:{}/{}'

class Container:
    # create a new container and return the wrapper
    @classmethod
    def create(cls, client, image_name, port, attr, max_wasm = 5):
        container = client.containers.run(image_name,
                                          detach=True,
                                          init=True,
                                          cpuset_cpus='0-2',
                                          cpu_quota=20000,
                                          ports={'8000/tcp': str(port)},
                                          labels=['workflow'])
        res = cls(container, port, attr, max_wasm)
        res.wait_start()
        return res

    # get the wrapper of an existed container
    # container_id is the container's docker id
    @classmethod
    def inherit(cls, client, container_id, port, attr):
        container = client.containers.get(container_id)
        return cls(container, port, attr)

    def __init__(self, container, port, attr, max_wasm):
        self.container = container
        self.port = port
        self.attr = attr
        self.lasttime = time.time()
        self.b = BoundedSemaphore()
        self.max_wasm = max_wasm
        self.num_exec = 0

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
            gevent.sleep(0.5)

    # send a request to container and wait for result
    def send_request(self, data = {}):

        self.b.acquire()
        self.num_exec += 1
        self.b.release()

        r = requests.post(base_url.format(self.port, 'run'), json=data)
        self.lasttime = time.time()

        self.b.acquire()
        self.num_exec -= 1
        self.b.release()

        print(r.json())
        return r.json()

    # initialize the container
    def init(self,function_name):
        data = {'function': function_name }
        r = requests.post(base_url.format(self.port, 'init'), json=data)
        self.lasttime = time.time()
        return r.status_code == 200

    # kill and remove the container
    def destroy(self):
        self.container.remove(force=True)
