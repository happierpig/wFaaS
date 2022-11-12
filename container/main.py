import docker
import gevent
from gevent import monkey
monkey.patch_all()
import time
from container import Container

# base_url = 'http://127.0.0.1:{}/{}'
# data = {'function': 'test', 'concurrency': 4}
# r1 = requests.get(base_url.format(24333,'status'))
# print("r1:",r1)
# print("status_code",r1.status_code)
# r2 = requests.post(base_url.format(24333, 'init'), json=data)
# print(r2)
# exit(0)

client = docker.from_env()
a = Container.create(client,'faas',23343,'exec')

a.init("test")
jobs = []
for i in range(10):
    print("Remote call : ",i)
    jobs.append(gevent.spawn(a.send_request))
gevent.joinall(jobs)

time.sleep(30)
a.destroy()
exit(0)
