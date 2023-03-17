from gevent import monkey
monkey.patch_all()
import time
import requests
import base64
import gevent


base_url = 'http://127.0.0.1:{}/{}'


input = b'\x01\x00\x00\x00\x02\x00\x00\x00'
encodedInput = base64.b64encode(input)

request_format="Test_{}"

def test_call(i):
    data = {'input':encodedInput, 'request_id':request_format.format(i)}
    r2 = requests.post(base_url.format(30000, 'run'), json=data)
    outputJson = r2.json()
    print(i,"th Call: ")
    print(outputJson)
    print(base64.b64decode(outputJson['output']))

jobs = []
for i in range(5):
    print("Remote run test:",i)
    jobs.append(gevent.spawn(test_call,i,))
gevent.joinall(jobs)
exit(0)