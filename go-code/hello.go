package main

/*
#cgo LDFLAGS: -ldl
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
int (*getNumSocketsWrap)();
int (*getMaxChannelWrap)();
void (*PCMMemoryInitWrap)();
void (*updateMemoryStatsWrap)();
void (*PCMMemoryFreeWrap)();
_Bool (*socketHasChannelWrap)(int, int);
float (*getChannelReadThroughputWrap)(int, int);
float (*getChannelWriteThroughputWrap)(int, int);
float (*getChannelPMMReadThroughputWrap)(int, int);
float (*getChannelPMMWriteThroughputWrap)(int, int);

void wrapInit() {
	void * handle = dlopen("./libcwrap.so", RTLD_LAZY);
	if(!handle) {
		printf("Abort: could not (dynamically) load shared library \n");
		exit(0);
	}
	getNumSocketsWrap = (int (*)()) dlsym(handle, "getNumSockets");
	getMaxChannelWrap = (int (*)()) dlsym(handle, "getMaxChannel");
	PCMMemoryInitWrap = (void (*)()) dlsym(handle, "PCMMemoryInit");
	updateMemoryStatsWrap = (void (*)()) dlsym(handle, "updateMemoryStats");
	PCMMemoryFreeWrap = (void (*)()) dlsym(handle, "PCMMemoryFree");
	socketHasChannelWrap = (_Bool (*)(int, int)) dlsym(handle, "socketHasChannel");
	getChannelReadThroughputWrap = (float (*)(int, int)) dlsym(handle, "getChannelReadThroughput");
	getChannelWriteThroughputWrap = (float (*)(int, int)) dlsym(handle, "getChannelWriteThroughput");
	getChannelPMMReadThroughputWrap = (float (*)(int, int)) dlsym(handle, "getChannelPMMReadThroughput");
	getChannelPMMWriteThroughputWrap = (float (*)(int, int)) dlsym(handle, "getChannelPMMWriteThroughput");

	PCMMemoryInitWrap();
}

void PCMMemoryInit() {
	PCMMemoryInitWrap();
}
void updateMemoryStats() {
	updateMemoryStatsWrap();
}
void PCMMemoryFree() {
	PCMMemoryFreeWrap();
}
float getChannelReadThroughput(int socket, int channel) {
	return getChannelReadThroughputWrap(socket, channel);
}
float getChannelWriteThroughput(int socket, int channel) {
	return getChannelWriteThroughputWrap(socket, channel);
}
float getChannelPMMReadThroughput(int socket, int channel) {
	return getChannelPMMReadThroughputWrap(socket, channel);
}
float getChannelPMMWriteThroughput(int socket, int channel) {
	getChannelPMMWriteThroughputWrap(socket, channel);
}
int getNumSockets() {
	return getNumSocketsWrap();
}
int getMaxChannel() {
	return getMaxChannelWrap();
}
_Bool socketHasChannel(int socket, int channel) {
	return socketHasChannelWrap(socket, channel);
}

*/
import "C"

import "fmt"
import "time"

func main() {
	C.wrapInit()
	defer C.PCMMemoryFree()
	
	socketNumber := C.getNumSockets()
	//fmt.Println("DEBUGDEBUGDEBUGDEBUGDEBUGDEBUG")
	maxChannel := C.getMaxChannel()
	
	for {
		time.Sleep(time.Duration(1) * time.Second)
		C.updateMemoryStats()
		for socket := 0; socket < int(socketNumber); socket++ {
			fmt.Println("------------------------------------")
			fmt.Println("Socket:", socket)
			for channel := 0; channel < int(maxChannel); channel++ {
				if !C.socketHasChannel(C.int(socket), C.int(channel)) {
					continue
				}
				fmt.Println("	Channel:", channel)
				fmt.Println("		Reads(MB/s):", C.getChannelReadThroughput(C.int(socket), C.int(channel)))
				fmt.Println("		Write(MB/s):", C.getChannelWriteThroughput(C.int(socket), C.int(channel)))
				fmt.Println("		PMM Reads(MB/s):", C.getChannelPMMReadThroughput(C.int(socket), C.int(channel)))
				fmt.Println("		PMM Writes(MB/s):", C.getChannelPMMWriteThroughput(C.int(socket), C.int(channel)))
			}
		}
		fmt.Println("------------------------------------")
		fmt.Println("")
	}
}