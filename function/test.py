from gevent import monkey
monkey.patch_all()
import time
import base64
import gevent
import functionUnit
import portController
import docker
import time

funcInfo = functionUnit.FunctionInfo("test", "test", "test", 100,100,100)
portMan = portController.PortController(20000,22000)

dockerClient = docker.from_env()
sessionScheduler = functionUnit.FunctionUnit(dockerClient, portMan, "Dreamer", funcInfo)

sessionScheduler.init()

input = b'\x01\x00\x00\x00\x02\x00\x00\x00'
encodedInput = base64.b64encode(input)

jobs = []
rqs = []
for i in range(30):
    # if i % 10 == 0:
    #     time.sleep(1)
    data = {'input':encodedInput, 'id':i}
    rq = functionUnit.RequestInfo(i, data, "Dreamer")
    rqs.append(rq)
    print("Remote run test:",i)
    jobs.append(gevent.spawn(sessionScheduler.send_request,rq))

gevent.joinall(jobs)
for i in range(30):
    print("Request ", i, " :", rqs[i].result.get())
time.sleep(100)
exit(0)
