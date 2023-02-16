from gevent import monkey
monkey.patch_all()
import time
import requests
import base64
import gevent


base_url = 'http://127.0.0.1:{}/{}'


data = {'function': 'test', 'id': 'thisisatest'}
r1 = requests.post(base_url.format(18000,'init'), json=data)
print("r1:",r1)
print("status_code",r1.status_code)
time.sleep(1)

input = b'\x01\x00\x00\x00\x02\x00\x00\x00'
encodedInput = base64.b64encode(input)

def test_call(i):
    data = {'input':encodedInput, 'id':i}
    r2 = requests.post(base_url.format(18000, 'run'), json=data)
    outputJson = r2.json()
    print(i,"th Call: ")
    print(outputJson)
    print(base64.b64decode(outputJson['output']))

jobs = []
for i in range(30):
    print("Remote run test:",i)
    jobs.append(gevent.spawn(test_call,i,))
gevent.joinall(jobs)
exit(0)