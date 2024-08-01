# Build

## 安装 [vcpkg](https://github.com/microsoft/vcpkg)

使用vcpkg安装布谷鸟哈希库[kuku](https://github.com/microsoft/Kuku)

```shell
git clone git@github.com:microsoft/vcpkg.git --depth 1
./vcpkg/bootstrap-vcpkg.sh
./vcpkg integrate install
./vcpkg install kuku
```

## iPrivjoin

```shell
git clone git@github.com:nhysteric/iPrivJoin.git --recursive
cd iPrivJoin/extern/volePSI
python3 build.py --debug -DVOLE_PSI_ENABLE_BOOST=ON  -DVOLE_PSI_ENABLE_OPPRF=ON  -DCMAKE_CXX_STANDARD=20
cd ../..
cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE=/home/nhy/vcpkg/scripts/buildsystems/vcpkg.cmake  -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE  -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++  -DCMAKE_CXX_STANDARD=20 -B build .
cmake --build build
```

将`DCMAKE_TOOLCHAIN_FILE`换成自己的vcpkg路径。如果不使用clang、clang++：`DCMAKE_CXX_COMPILER`与`DCMAKE_C_COMPILER`也要更换。确保编译器支持c++20

## Run

```shell
./build/iPrivJoin {config名}
```

在`test/config`下配置参数文件，如下所示：

```toml
bins = 16384
funcs = 2
address = "localhost:8011"

pa_elems = 15500
pa_features = 15
pb_elems = 15500
pb_features = 14
max_in_bin = 3
```

 ~~字段具体含义不解释了。~~

 实例：

 ```shell
 ./build/iPrivJoin application
 ``` 

 