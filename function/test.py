from gevent import monkey
monkey.patch_all()
import time
import base64
import gevent
import functionUnit
import container
import portController
import docker
import time
import sys
import numpy as np

resultFilePath = sys.argv[1]
resultFile = open(resultFilePath, "w")

funcInfo = functionUnit.FunctionInfo("test", "test", "test", 20,100,100)
portMan = portController.PortController(30000,45000)
dockerClient = docker.from_env()

benchArr = ["centr_avg", "matrix_mul", "data_access", "data_access_linear"]
closeLoopArr = [1, 5, 10, 20, 30, 40, 50, 60, 70, 80, 90]
runTime = 60

globalLatencyBuffer = []

def closeLoopTester(sessionScheduler, startRunTime, inputData, senderIndex):
    global runTime
    global globalLatencyBuffer
    endTime = time.time()
    tmpCount = 0
    while endTime - startRunTime < runTime:
        tmpCount += 1
        data = {'input':inputData, 'request_id':f"Test_i{senderIndex}_c{tmpCount}"}
        rq = functionUnit.RequestInfo(data['request_id'], data, "Dreamer")
        sessionScheduler.send_request(rq)
        runningTime = (rq.result.get())['twoPartTime']
        globalLatencyBuffer.append(int(runningTime))
        endTime = time.time()

benchBasePath = "/xfaas/experiments/testcase"
inputData = None
for bench in benchArr:
    if bench == "data_access" or bench == "data_access_linear":
        inputData = b'\x01\x00\x00\x00'
    if bench == "matrix_mul":
        inputData = b'\x01\x00\x00\x00\x02\x00\x00\x00'
    if bench == "centr_avg":
        inputData = b'\x01\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00'
    encodedInput = base64.b64encode(inputData).decode('utf-8')
    for closeLoopSize in closeLoopArr:
        globalLatencyBuffer = []
        sessionScheduler = functionUnit.FunctionUnit(dockerClient, portMan, "Dreamer",
                                                    funcInfo, benchBasePath+"/"+bench, isOss=False, parrelWorker=10)
        sessionScheduler.init()
        time.sleep(2)
        # launch 30 new gevent threads with closeLoopTester()
        threads = []
        for i in range(closeLoopSize):
            threads.append(gevent.spawn(closeLoopTester, sessionScheduler, time.time(), encodedInput, i))
        gevent.joinall(threads)
        sessionScheduler.destroy()

        globalLatencyBuffer = globalLatencyBuffer[-2000:]
        resultFile.write(f"{bench},{closeLoopSize},mem,{(int)(1e6 / np.mean(globalLatencyBuffer) * closeLoopSize)}\n")
        resultFile.flush()
    for closeLoopSize in closeLoopArr:
        globalLatencyBuffer = []
        sessionScheduler = functionUnit.FunctionUnit(dockerClient, portMan, "Dreamer",
                                                    funcInfo, benchBasePath+"/"+bench, isOss=True, parrelWorker=10)
        sessionScheduler.init()
        time.sleep(2)
        # launch 30 new gevent threads with closeLoopTester()
        threads = []
        for i in range(closeLoopSize):
            threads.append(gevent.spawn(closeLoopTester, sessionScheduler, time.time(), encodedInput, i))
        gevent.joinall(threads)
        sessionScheduler.destroy()

        globalLatencyBuffer = globalLatencyBuffer[-2000:]
        resultFile.write(f"{bench},{closeLoopSize},oss,{(int)(1e6 / np.mean(globalLatencyBuffer) * closeLoopSize)}\n")
        resultFile.flush()