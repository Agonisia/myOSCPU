# OSCPU: Open-Source Chip Project by University

OSCPU is an open-source chip project, mainly organized by University of Chinese Academy of Science (UCAS) and Institute of Computing Technology (ICT). 

This project aims to lower the threshold of processor chip design through a teaching mechanism that emphasizes both theory and practice, let students participate in every aspect of processor chip design.

OSCPU is open to all enthusiasts regardless of age or major. Students can apply for tape-out after achieving a certain criteria, current students in China can get free tape-out.

You can find more infomation about this project on [Official Website]

## Project progress

This repository only serves as a presentation. Many implementations might remain flawed. 

Subsequent development will not be immediately posted in the public repository

The current pace is the successful boot of rt-thread. 

I wish to convert my self-designed processor to a GPU after certain requirements are met.

## Repository structure
`nemu`: <u>n</u>ju <u>emu</u>lator, a lightweight simulator developed for teaching purposes. Can switch between different ISA architectures via macro settings.

`npc`: <u>n</u>ative <u>p</u>rocessor <u>c</u>ore, a processor of my own design and development, based on the chisel language.

`abstract-machine`: provide runtime environment for `nemu` and `npc`

`yosys-sta`: provide static analysis for `npc` to help analyze design and subsequent optimization

`am-kernels`: the kernel of `abstract-machine`, provide instruction test sets and peripheral inspections for `nemu` and `npc`. [Origin Repo](https://github.com/NJU-ProjectN/am-kernels.git)

`rt-thread-am`: rt-thread system specially adapted for `abstract-machine`, can run on `nemu` and `npc`. [Origin Repo](https://github.com/NJU-ProjectN/am-kernels.git)

`nvboard`: A virtual FPGA board developed based on SDL, can simulate FPGA in Verilator.

`fceux-am`: A NES emulator for `abstract-machine`.


## Preparatory work

### Environment Variable 
```bash
--- ~/.bashrc
+++ ~/.bashrc

export NEMU_HOME=/path/to/nemu
export AM_HOME=/path/to/abstract-machine
export NVBOARD_HOME=/path/to/nvboard
export NPC_HOME=/path/to/npc
```

### Dependency Installation
```bash
apt install build-essential man gcc-doc gdb git libreadline-dev libsdl2-dev bison flex g++-riscv64-linux-gnu binutils-riscv64-linux-gnu device-tree-compiler scons
```

I believe after many times of reconfiguring the environment, I shouldn't forget certain packages. If they are something still missing, please refer to the [Project Handout]

### Compilation Error

If following error is reported when compiling the dummy program.

```bash
/usr/riscv64-linux-gnu/include/bits/wordsize.h:28:3: error: #error "rv32i-based targets are not supported"
```

Modify the following files with sudo privileges:

```bash
--- /usr/riscv64-linux-gnu/include/bits/wordsize.h
+++ /usr/riscv64-linux-gnu/include/bits/wordsize.h
@@ -25,5 +25,5 @@
 #if __riscv_xlen == 64
 # define __WORDSIZE_TIME64_COMPAT32 1
 #else
-# error "rv32i-based targets are not supported"
+# define __WORDSIZE_TIME64_COMPAT32 0
 #endif
```

If the following error is reported.

```bash
/usr/riscv64-linux-gnu/include/gnu/stubs.h:8:11: fatal error: gnu/stubs-ilp32.h: No such file or directory
```

Modify the following files with sudo privileges.

```bash
--- /usr/riscv64-linux-gnu/include/gnu/stubs.h
+++ /usr/riscv64-linux-gnu/include/gnu/stubs.h
@@ -5,5 +5,5 @@
 #include <bits/wordsize.h>

 #if __WORDSIZE == 32 && defined __riscv_float_abi_soft
-# include <gnu/stubs-ilp32.h>
+//# include <gnu/stubs-ilp32.h>
 #endif
```


[Official Website]: https://ysyx.oscc.cc/

[Project Handout]: https://ysyx.oscc.cc/docs/
