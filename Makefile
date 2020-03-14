c++ = g++
c++flags = -std=c++17 -DDEBUG -O3
out = bin/codar
analyzer = bin/analyzer
grid_simulator = bin/grid_simulatormkae

$(out): build/BasicGridDevice.o build/SimpleGridDevice.o build/UncompletedGridDevice.o build/ExGridDevice.o build/QScheduler.o build/QASMParser.o build/main.o 
	@mkdir -p bin
	$(c++) $(c++flags) $^ -o $(out)

build/BasicGridDevice.o: src/BasicGridDevice.cpp src/BasicGridDevice.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/SimpleGridDevice.o: src/SimpleGridDevice.cpp src/SimpleGridDevice.h src/BasicGridDevice.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/ExGridDevice.o: src/ExGridDevice.cpp src/ExGridDevice.h src/BasicGridDevice.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/IdealGridDevice.o: src/IdealGridDevice.cpp src/IdealGridDevice.h src/BasicGridDevice.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/QScheduler.o: src/QScheduler.cpp src/QScheduler.h src/BasicGridDevice.h src/basis.h src/commuting.def
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/QScheduler_no_swapping.o: src/QScheduler.cpp src/BasicQScheduler.h src/GridDevice.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -DNO_SWAPPING -c $< -o $@

build/QASMParser.o: src/QASMParser.cpp src/QASMParser.h src/basis.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/UncompletedGridDevice.o: src/UncompletedGridDevice.cpp src/UncompletedGridDevice.h src/SimpleGridDevice.h src/BasicGridDevice.h
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

build/main.o: src/main.cpp src/QASMParser.h src/QASMParser.h src/BasicGridDevice.h src/SimpleGridDevice.h src/UncompletedGridDevice.h src/ExGridDevice.h src/basis.h arch/qpair/ibm_q53.qpair arch/qpair/tokyo.qpair
	@mkdir -p build
	$(c++) $(c++flags) -c $< -o $@

$(analyzer): src/basis.h src/QAnalyzer.cpp src/QASMParser.o
	@mkdir -p bin
	$(c++) $(c++flags) src/QAnalyzer.cpp build/QASMParser.o -o $(analyzer)

$(grid_simulator): src/basis.h src/QGridSimulator.cpp build/IdealGridDevice.o build/BasicGridDevice.o build/QASMParser.o 
	@mkdir -p bin
	$(c++) $(c++flags) src/QGridSimulator.cpp build/IdealGridDevice.o build/QASMParser.o build/BasicGridDevice.o -o $(grid_simulator)

.PHONY: clean all

all: $(out) $(analyzer) $(grid_simulator)

clean:
	-rm build/*
	-rm $(out)
	-rm $(analyzer)
	-rm $(grid_simulator)

