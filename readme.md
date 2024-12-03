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

Clone:

```shell
git clone git@github.com:nhysteric/iPrivJoin.git --recursive
```

Change `extern/volePSI/volePSI/SimpleIndex.cpp:276` 

```cpp
#if !defined(NDEBUG) && defined(ENABLE_BOOST)
                auto B2 = get_bin_size(numBins, numBalls, statSecParam, false);
                assert(B2 <= B);
#endif
                return B;
            }
        }
```

to 

```cpp
#if !defined(NDEBUG) && defined(ENABLE_BOOST)
                auto B2 = get_bin_size(numBins, numBalls, statSecParam, false);
                // assert(B2 <= B);
                if (B2 > B)
                    return B2;
                else
                    return B;
#endif
            }
        }
```

Than compile:

```shell
cd iPrivJoin/extern/volePSI
python3 build.py -DVOLE_PSI_ENABLE_BOOST=ON
cd ../..
cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE=/home/nhy/vcpkg/scripts/buildsystems/vcpkg.cmake  -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE  -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++  -DCMAKE_CXX_STANDARD=20 -B build .
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
```

 ~~字段具体含义不解释了。~~

 实例：

 ```shell
 ./build/iPrivJoin application wan
 ``` 

 