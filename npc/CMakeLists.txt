cmake_minimum_required(VERSION 3.10)

project(RockBottom LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# basic configs
set(OP_NAME Core)
set(GTKWAVE gtkwave)
set(VERILATOR verilator)
set(TOP_NAME Core)

# directories setup
set(BUILD_DIR ${CMAKE_BINARY_DIR}/build)
set(OBJ_DIR ${BUILD_DIR}/obj_dir)
set(BIN ${BUILD_DIR}/${TOP_NAME})
set(CHISEL_DIR ${CMAKE_SOURCE_DIR}/chisel/verilog)
set(VSRC_DIR ${CMAKE_SOURCE_DIR}/vsrc)
set(CSRC_DIR ${CMAKE_SOURCE_DIR}/csrc)
set(SYNC_MARKER ${BUILD_DIR}/.chisel_sync)
set(VERILATOR_PATH "/usr/local/share/verilator/include")
set(SIMULATE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/csrc/1nclude)

# make sure the build directories exist
file(MAKE_DIRECTORY ${BUILD_DIR})

# get source files
file(GLOB_RECURSE VSRC "${VSRC_DIR}/*.v" "${VSRC_DIR}/*.sv")
file(GLOB_RECURSE CSRC "${CSRC_DIR}/*.c" "${CSRC_DIR}/*.cc" "${CSRC_DIR}/*.cpp")

# set include paths
set(INC_PATH
  ${SIMULATE_PATH}
  ${OBJ_DIR}
)

# transform the paths to include format
list(TRANSFORM INC_PATH PREPEND "-I")

# set top module name
set(TOP_NAME_DEFINE "-DTOP_NAME=\\\"V${TOP_NAME}\\\"")

# get llvm flags
execute_process(
  COMMAND llvm-config --cxxflags
  OUTPUT_VARIABLE LLVM_CXXFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\n" " " LLVM_CXXFLAGS "${LLVM_CXXFLAGS}")
separate_arguments(LLVM_CXXFLAGS)

execute_process(
  COMMAND llvm-config --libs --ldflags
  OUTPUT_VARIABLE LLVM_LIBS
  OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\n" " " LLVM_LIBS "${LLVM_LIBS}")
separate_arguments(LLVM_LIBS)

execute_process(
  COMMAND llvm-config --libdir
  OUTPUT_VARIABLE LLVM_LIBDIR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

# set compile flags
set(CXX_FLAGS_LIST
  ${INC_PATH}
  -DTOP_NAME="VCore"
  ${LLVM_CXXFLAGS}
  -fPIE
)
string(JOIN " " CXX_FLAGS_STR ${CXX_FLAGS_LIST})

set(LD_FLAGS_LIST
  -L${LLVM_LIBDIR}
  ${LLVM_LIBS}
  -lreadline
)
string(JOIN " " LD_FLAGS_STR ${LD_FLAGS_LIST})

set(VERILATOR_FLAGS 
  -MMD --build -cc
  -O3 --x-assign fast --x-initial fast --noassert
  --trace-fst
)

add_custom_command(
  OUTPUT ${BIN}
  COMMAND ${VERILATOR} ${VERILATOR_FLAGS}
    --top-module ${TOP_NAME}
    ${VSRC} ${CSRC}
    "-CFLAGS" ${CXX_FLAGS_STR}
    "-LDFLAGS" ${LD_FLAGS_STR}
    --Mdir ${OBJ_DIR}
    --exe -o ${BIN}
  DEPENDS ${VSRC} ${CSRC}
  COMMENT "Building with Verilator..."
  VERBATIM
)

# build verilator (make)
add_custom_target(build_verilator ALL DEPENDS ${BIN})

# sync
add_custom_target(sync
  COMMAND rsync -av --delete --ignore-times ${CHISEL_DIR}/ ${VSRC_DIR}/
  COMMAND touch ${SYNC_MARKER}
  COMMENT "Syncing Chisel generated Verilog..."
)

# build full (make all)
add_custom_target(build_full DEPENDS sync build_verilator)

# set dynamic library path
set(DIFF_REF_SO $ENV{NEMU_HOME}/build/riscv32-nemu-interpreter-so)

# set simulation arguments
set(ARGS -d ${DIFF_REF_SO})
if (IMG)
  set(ARGS "${ARGS} ${IMG}")
endif()

# sim
# usage: enter build and cmake --build . --target sim
add_custom_target(sim
  COMMAND ${BIN} ${ARGS}
  COMMENT "Running simulation with args: ${ARGS}"
)

# clean all (make clean)
add_custom_target(clean_all
  COMMAND rm -rf ${BUILD_DIR} sim_record.fst
  COMMENT "Cleaning build artifacts...") 

# sbt
add_custom_target(sbt
  COMMAND cd ${CMAKE_SOURCE_DIR}/chisel && sbt run
  COMMENT "Running Chisel with sbt..."
)

# mill
add_custom_target(mill
  COMMAND cd ${CMAKE_SOURCE_DIR}/chisel && mill -i RockBottom.runMain core.Core
  COMMENT "Running Chisel with mill..."
)

# gtkwave
add_custom_target(fst
  COMMAND ${GTKWAVE} sim_record.fst
  COMMENT "Opening waveform with GTKWave..."
)