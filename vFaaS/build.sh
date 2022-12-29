cd wasmCode
bash build.sh
cd ../build
make clean
cmake ..
make
./vFaaS