TOPNAME = Core
INC_PATH ?=
GTKWAVE = gtkwave

VERILATOR = verilator
VERILATOR_FLAGS += -MMD --build -cc 
VERILATOR_FLAGS += -O3 --x-assign fast --x-initial fast --noassert 
VERILATOR_FLAGS += --trace-fst

BUILD_DIR = ./build
OBJ_DIR = $(BUILD_DIR)/obj_dir
BIN = $(BUILD_DIR)/$(TOPNAME)

CHISEL_DIR = ./chisel/verilog
VSRC_DIR = ./vsrc
CSRC_DIR = ./csrc
SYNC_MARKER = $(BUILD_DIR)/.chisel_sync

$(shell mkdir -p $(BUILD_DIR))

# project source
VSRC = $(shell find $(abspath $(VSRC_DIR)) -name "*.v" -or -name "*.sv")
CSRC = $(shell find $(abspath $(CSRC_DIR)) -name "*.c" -or -name "*.cc" -or -name "*.cpp")

# rules for verilator
INCFLAGS = $(addprefix -I, $(INC_PATH))
CXXFLAGS += $(INCFLAGS) -DTOP_NAME="\"V$(TOPNAME)\""

# sync chisel/verilog to vsrc
$(SYNC_MARKER): 
	@echo "Syncing Chisel-generated Verilog to vsrc..."
	@rsync -av --delete $(CHISEL_DIR)/ $(VSRC_DIR)/
	@touch $(SYNC_MARKER)

$(BIN): $(SYNC_MARKER) $(VSRC) $(CSRC)
	@echo "Cleaning old object directory"
	@rm -rf $(OBJ_DIR)
	@echo "Compiling with Verilator"
	$(VERILATOR) $(VERILATOR_FLAGS) \
		--top-module $(TOPNAME) $^ \
		$(addprefix -CFLAGS , $(CXXFLAGS)) $(addprefix -LDFLAGS , $(LDFLAGS)) \
		--Mdir $(OBJ_DIR) --exe -o $(abspath $(BIN))

sim: $(BIN)
	$(call git_commit, "sim RTL") # DO NOT REMOVE THIS LINE!!!
	@echo "Running simulation..."
	@$^

clean:
	@echo "Cleaning up build artifacts"
	@rm -rf $(BUILD_DIR) result/sim_record.fst 

.PHONY: sim clean

include ../Makefile