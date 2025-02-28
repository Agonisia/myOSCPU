TOP_NAME = Core
INC_PATH ?=
GTKWAVE = gtkwave
export CLK_FREQ_MHZ ?= 500

VERILATOR = verilator
VERILATOR_FLAGS += -MMD --build -cc 
VERILATOR_FLAGS += -O3 --x-assign fast --x-initial fast --noassert 
VERILATOR_FLAGS += --trace-fst

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOP_NAME)

CHISEL_DIR = ./chisel/verilog
VSRC_DIR = ./vsrc
CSRC_DIR = ./csrc
SYNC_MARKER = $(BUILD_DIR)/.chisel_sync

RESULT_DIR = $(NPC_HOME)/result/$(TOP_NAME)-$(CLK_FREQ_MHZ)MHz
SCRIPT_DIR = $(NPC_HOME)/scripts
NETLIST_SYN_V   = $(RESULT_DIR)/$(TOP_NAME).netlist.syn.v
NETLIST_FIXED_V = $(RESULT_DIR)/$(TOP_NAME).netlist.fixed.v
TIMING_RPT = $(RESULT_DIR)/$(TOP_NAME).rpt
SDC_FILE ?= $(SCRIPT_DIR)/$(TOP_NAME).sdc

$(shell mkdir -p $(BUILD_DIR))

# project source
VSRC = $(shell find $(abspath $(VSRC_DIR)) -name "*.v" -or -name "*.sv")
CSRC = $(shell find $(abspath $(CSRC_DIR)) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

# include paths
INC_PATH += $(abspath ./csrc/1nclude)
INC_FLAGS = $(addprefix -I, $(INC_PATH))

# compiler flags
CXX_FLAGS += $(INC_FLAGS) -DTOP_NAME="\"V$(TOP_NAME)\""
CXX_FLAGS += $(shell llvm-config --cxxflags) -fPIE
LD_FLAGS += $(shell llvm-config --libs) -lreadline -lSDL2

# dynamic library path
DIFF_REF_SO = $(NEMU_HOME)/build/riscv32-nemu-interpreter-so
ARGS += -d $(DIFF_REF_SO)

$(BIN): $(VSRC) $(CSRC)
	@echo "Cleaning old object directory"
	@rm -rf $(OBJ_DIR)
	@echo "Compiling with Verilator"
	$(VERILATOR) $(VERILATOR_FLAGS) \
		--top-module $(TOP_NAME) $^ \
		$(addprefix -CFLAGS , $(CXX_FLAGS)) $(addprefix -LDFLAGS , $(LD_FLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

# sync chisel generated verilog
sync:
	@echo "Force syncing Chisel-generated Verilog to vsrc..."
	@rsync -av --delete --ignore-times $(CHISEL_DIR)/ $(VSRC_DIR)/
	@touch $(SYNC_MARKER)

# full build with sync
all: sync $(BIN)

# run simulation
sim: 
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@if [ -n "$(IMG)" ]; then \
		echo "Running simulation with arguments: "$(ARGS)" and image: $(IMG)"; \
		$(BIN) $(ARGS) $(IMG); \
	else \
		echo "Running simulation with arguments: "$(ARGS)" in default mode"; \
		$(BIN) $(ARGS); \
	fi

# clean up build artifacts
clean:
	@echo "Cleaning up build artifacts"
	@rm -rf $(BUILD_DIR) sim_record.fst 

# run chisel with sbt
sbt:
	@cd ./chisel && sbt run

# run chisel with mill
mill:
	@cd ./chisel && mill -i RockBottom.runMain core.Core

# open generated waveform
fst:
	$(GTKWAVE) sim_record.fst

# synthesis design
syn: $(NETLIST_SYN_V)
$(NETLIST_SYN_V): $(VSRC) $(SCRIPT_DIR)/yosys.tcl
	mkdir -p $(@D)
	echo tcl $(SCRIPT_DIR)/yosys.tcl $(TOP_NAME) \"$(VSRC)\" $@ | yosys -l $(@D)/yosys.log -s -

# fix fanout violation in design
fix-fanout: $(NETLIST_FIXED_V)
$(NETLIST_FIXED_V): $(SCRIPT_DIR)/fix-fanout.tcl $(SDC_FILE) $(NETLIST_SYN_V)
	./bin/iEDA -script $^ $(TOP_NAME) $@ 2>&1 | tee $(RESULT_DIR)/fix-fanout.log

# static timing analysis design
sta: $(TIMING_RPT)
$(TIMING_RPT): $(SCRIPT_DIR)/sta.tcl $(SDC_FILE) $(NETLIST_FIXED_V)
	./bin/iEDA -script $^ $(TOP_NAME) 2>&1 | tee $(RESULT_DIR)/sta.log

.PHONY: sync all sim clean sbt mill fst syn fix-fanout sta

include ../Makefile