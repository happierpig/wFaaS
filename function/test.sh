python3 test.py
docker stop $(docker ps -a -q)
docker  rm $(docker ps -a -q)