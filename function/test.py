from gevent import monkey
monkey.patch_all()
import time
import base64
import gevent
import functionUnit
import portController
import docker
import time

# input = b'\x01\x00\x00\x00\x02\x00\x00\x00'
# integer = 3
# input = input + integer.to_bytes(4, 'little')
# print(integer.to_bytes(4, 'little'))
# exit(0)

funcInfo = functionUnit.FunctionInfo("test", "test", "test", 100,100,100)
portMan = portController.PortController(30000,32000)

dockerClient = docker.from_env()
sessionScheduler = functionUnit.FunctionUnit(dockerClient, portMan, "Dreamer", funcInfo)

sessionScheduler.init()

input = b'\x01\x00\x00\x00\x02\x00\x00\x00'
encodedInput = base64.b64encode(input)

jobs = []
rqs = []
testNum = 30
request_format="Test_{}"
for i in range(testNum):
    time.sleep(0.005)
    data = {'input':encodedInput, 'request_id':request_format.format(i)}
    rq = functionUnit.RequestInfo(i, data, "Dreamer")
    rqs.append(rq)
    print("Remote run test:",i)
    jobs.append(gevent.spawn(sessionScheduler.send_request,rq))

gevent.joinall(jobs)
for i in range(testNum):
    resJson = rqs[i].result.get()
    print("Request ", i, " :", resJson," ; sum value:",int.from_bytes(base64.b64decode(resJson['output']),byteorder="little"))
# time.sleep(10)
exit(0)
