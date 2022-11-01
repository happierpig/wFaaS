from gevent import monkey ;monkey.patch_all()
import docker
import gevent
from container import Container

client = docker.from_env()
a = Container.create(client,'faas',30000,'exec')

if a.init('gcd') :
    print("Call init success.")
else:
    print("Fail to init.")
    exit(0)

jobs = []
for i in range(10):
    print("Remote call : ",i)
    jobs.append(gevent.spawn(a.send_request))
gevent.joinall(jobs)
