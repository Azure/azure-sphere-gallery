cmake --preset ARM-Debug
cmake --build out/ARM-Debug
azsphere device sideload delete
azsphere device sideload deploy -p out/ARM-Debug/HTTPS_MutualAuth.imagepackage -m
azsphere device app start --debug-mode --component-id 5d7c4db5-d182-4ccc-9a23-1bdc6d899cb6

echo nc 192.168.35.2 2342
echo gdb-multiarch out/ARM-Debug/HTTPS_MutualAuth.out -ex "target remote 192.168.35.2:2345" -ex c