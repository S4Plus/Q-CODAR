# CODAR: A Contextual Duration-Aware Qubit Mapping for Various NISQ Devices

This project provides an OpenQASM source-to-source compiler which supports optimization for different NISQ (Noisy Intermediate-Scale Quantum) devices. It currently contains the CODAR qubit mapping algorithm accepted by [DAC 2020](https://www.dac.com/). 
The prototype was developed in C++ by [Quanxi Li](https://github.com/Crazylqx) and [Haowei Deng](https://github.com/sqrta) under the guidance of Prof. [Yu Zhang](http://staff.ustc.edu.cn/~yuzhang) in the [S4Plus](https://s4plus.ustc.edu.cn/) Quantum team at [USTC](https://www.ustc.edu.cn/).

The CODAR (COntext-sensitive and Duration-Aware Remapping algorithm) remapper is built on the abstraction of different NISQ devices, called Multi-architecture Adaptive Quantum Abstract Machine (maQAM), and is aware of gate duration difference and quantum program context.
You can read the [preprint](https://arxiv.org/abs/2002.10915) for details of the maQAM and the CODAR algorithm.

If you find the CODAR useful in your research, please consider citing:
```
@inproceedings{ deng2020codar,
    title	= {{CODAR}: A Contextual Duration-Aware Qubit Mapping for Various {NISQ} Devices},
    author	= {Deng, Haowei and Zhang, Yu and Li, Quanxi},
    booktitle={Proceedings of 2020 57th ACM/IEEE Design Automation Conference (DAC)},
	month	= {July},
    year	={2020},
    organization={IEEE},
    doi		= {10.1109/DAC18072.2020.9218561},
    url		= {https://ieeexplore.ieee.org/document/9218561},
}
```

## Building

- This project should be built and run on \*nix system.
- Run `make` to build the CODAR source-to-source compiler.
- Run `make all` to build the compiler, the qasm analyzer tool and the simulator for debugging.
- All executable files will be generated in directory `bin/`.

## Compiling OpenQASM Programs

First please build the CODAR compiler.
By default, the path of it will be `bin/codar`.

**Note**: CODAR does not support flattening a program and make sure the `module` syntax is not used in your OpenQASM program;
otherwise please flatten the code before compiling.

Run
```
    bin/codar
```
and you will get how to use CODAR as follows
```
usage: ./codar <filename> [args]
args:
        -o <out_file>   default: output/*.qasm  set output file name
        -m <M>          default: 2              set side length for grid device
        -n <N>          default: 8              set side length for grid device
        -r <run_time>   default: 1              run <run_time> times and output the best one
        -init <file>                            use a specific initial mapping from file
        --init                                  use a specific initial mapping from stdin
        --tokyo                                 use ibm-tokyo 20-qubit device model
        --ibm_q53                               use ibm-Rochester 53-qubit device model
        --google_q54                            use google-Sycamore 54-qubit device model
```

## Example

**Compile an OpenQASM program based on a specified grid model**
```
    bin/codar before.qasm -m 6 -n 6 -r 10 -o after.qasm
```
You will compile `before.qasm` which is unfit to the device qubit coupling limitation on a "6 x 6" grid device model for 10 times.
You will get output to `after.qasm` which fits the coupling limitation, 
and the corresponding initial mapping `after.qasm.map`.

**Compile program based on other supported model**
```
bin/codar before.qasm --tokyo -r 10 -o after.qasm
```
you will compile your program on IBM-Tokyo 20-qubit device model instead of a grid model.

**Compile program with a custom initial mapping**
```
bin/codar before.qasm -m 6 -n 6 -o after.qasm --init
```
Then you enter your initial mapping
```
please input initial mapping:
0 => 2; 1 => 8; 2 => 7; 3 => 0; 4 => 5;
```
which means you map the first qubit to q[2], the second to q[8], etc.
In 6x6 gird device, q[2] is at (0, 2) and q[8] is at (1, 2).
In general, q[i] is at (i / 6, i % 6).

If you hate to enter such inital mapping in the comand line every time,
you can save it to a file and run
```
bin/codar before.qasm -m 6 -n 6 -o after.qasm -init /path/to/initial/mapping/file
```
and get a same effect.

Attention that if you also set `-r` for more than once,
We will iteratively improve the initial mapping and sometimes give you a better one.


Have fun with CODAR!

## License
CODAR is liscensed under the [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0).


