import time
import requests

base_url = 'http://127.0.0.1:{}/{}'


data = {'function': 'test', 'concurrency': 4}
r1 = requests.post(base_url.format(18000,'init'), json=data)
print("r1:",r1)
print("status_code",r1.status_code)
print(r1.json())
r2 = requests.post(base_url.format(18000, 'test'), json=data)
print(r2.json())
exit(0)