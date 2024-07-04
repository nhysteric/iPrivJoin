cmake_minimum_required(VERSION 3.19)

set(CMAKE_TOOLCHAIN_FILE /home/nhy/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

project(iPrivJoin VERSION 0.1.0)

set(CUSTOM_NAME iPrivJoin)
set(libOTe_DIR extern/volePSI/out/libOTe)
set(volePSI_DIR extern/volePSI)

find_package(volePSI HINTS ${volePSI_DIR} REQUIRED)
find_package(libOTe HINTS ${libOTe_DIR} REQUIRED)
find_package(Kuku CONFIG REQUIRED)

aux_source_directory(src DIR_SRC)
aux_source_directory(src/shuffle DIR_SHUFFLE)
aux_source_directory(src/ggm DIR_GGM)
aux_source_directory(src/joinData DIR_JOINDATA)
aux_source_directory(src/opprf DIR_OPPRF)
aux_source_directory(src/oprf DIR_OPRF)

add_executable(
  ${CUSTOM_NAME}
  main.cpp
  ${DIR_OPRF}
  ${DIR_OPPRF}

  # ${DIR_SRC}
  # ${DIR_SHUFFLE}
  # ${DIR_GGM}
  ${DIR_JOINDATA}
)




include_directories(extern src/shuffle src/ggm src/joinData src/opprf src/oprf )
target_link_libraries(${CUSTOM_NAME} visa::volePSI oc::libOTe Kuku::kuku)
target_compile_options(${CUSTOM_NAME} PUBLIC -march=native -std=c++20) 
target_include_directories(${CUSTOM_NAME} PUBLIC ${volePSI_DIR})