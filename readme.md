An unofficial implementation of [iPrivJoin](https://ieeexplore.ieee.org/document/10159165/)

# Build

## Install [vcpkg](https://github.com/microsoft/vcpkg)

Install [kuku](https://github.com/microsoft/Kuku) by `vcpkg`

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
cmake --no-warn-unused-cli -DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake  -DCMAKE_BUILD_TYPE:STRING=Release -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE  -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++  -DCMAKE_CXX_STANDARD=20 -B build .
cmake --build build
```

Change `DCMAKE_TOOLCHAIN_FILE` to your path of `vcpkg`,and make sure your compiler supports C++20.

## Run

```shell
./build/iPrivJoin {config_name} {env_type}
```
A config is a TOML file, with fields and format as follows:

```toml
bins = 16384
funcs = 2
address = "localhost:8011"

pa_elems = 15500
pa_features = 15
pb_elems = 15500
pb_features = 14
```

Example:

 ```shell
 ./build/iPrivJoin example lan
 ``` 

You will find the output files under `test/lan`.

 