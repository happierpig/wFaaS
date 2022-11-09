from gevent import monkey ;monkey.patch_all()
import docker
import requests
import gevent
from container import Container

base_url = 'http://127.0.0.1:{}/{}'
data = {'function': 'test', 'concurrency': 4}
r = requests.post(base_url.format(24333, 'init'), json=data)
print(r)
exit(0)

client = docker.from_env()
a = Container.create(client,'faas',23343,'exec')

a.init("test")
exit(0)
