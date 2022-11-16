from gevent import monkey
monkey.patch_all()
import docker
import gevent
import time
import requests
from container import Container

def test_call(_i):
    data2 = {"input": 'wuwu'}
    r = requests.post(base_url.format(23333, 'run'), json=data2)
    print(_i,"th Call:",r.json())

base_url = 'http://127.0.0.1:{}/{}'
data = {'function': 'test', 'concurrency': 4}
r1 = requests.get(base_url.format(23333,'status'))
print("r1:",r1)
print("status_code",r1.status_code)
r2 = requests.post(base_url.format(23333, 'init'), json=data)
print(r2.json())
time.sleep(5)
jobs = []
for i in range(10):
    print("Remote run test:",i)
    time.sleep(1)
    jobs.append(gevent.spawn(test_call,i,))
gevent.joinall(jobs)
exit(0)

client = docker.from_env()
a = Container.create(client,'faas',23343,'exec')

a.init("test")
exit(0)

jobs = []
for i in range(10):
    print("Remote call : ",i)
    jobs.append(gevent.spawn(a.send_request))
gevent.joinall(jobs)

time.sleep(30)
a.destroy()
exit(0)
