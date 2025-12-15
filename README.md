# Lock Free Snowflake
A novel, lock-free, multithreaded implementation of a unique, roughly time-ordered, 64-bit id generator for use in distributed server and database systems.

## Contents
1. [Abstract](#abstract)
2. [Terminology](#terminlogy)
2. [Design Constraints](#design-constraints)
3. [Implementation Basics](#implementation-basics)
4. [Implementation Details](#implementation-details)
4. [Results](#results)
5. [Takeaways](#takeaways)
6. [Building](#building)
7. [Testing](#testing)

## Abstract
Snowflake IDs are a 64-bit unique number that can be used to represent any object within distributed server and database systems. A snowflake is generally comprised of three parts: a timestamp, a machine-process ID, and a per-millisecond sequence number. Using this type of format avoids unnecessary synchronization between servers in different parts of the world.

Because snowflakes contain a timestamp in ms, each ID is therefore roughly time-sortable.

Most methods of generating snowflakes utilize a single daemon running a single snowflake generation algorithm. Threads running other parts of the server and database code therefore do not need to worry about the uniqueness of a given id because they all query the daemon thread for unique ids.

Using a separate thread for only generating Snowflake IDs is a waste of resources, and carries a massive overhead with running interprocess socket logic to request and transfer Snowflakes.

My approach to this problem is remove the use of a daemon thread entirely and have each thread handle the logic of generating snowflakes itself. This removes the overhead of interprocess communication to a single daemon for generating unique IDs. In my approach, I make use of atomic snchronization techniques to achieve low overhead, high throughput, and maintain the same behavior as the aforementioned approach.

## Terminlogy
**Snowfake ID** - a unique, roughly time-ordered, 64-bit unsigned integer identifier for use in database systems.

**Output Sequence** - a sequence of Snowflake IDs that are produced by the algorithm. 

**Server** - a multithreaded machine (or collection of such) running one or more server application instances

**Process** - an instance of a server application running on a server

**Timestamp** - time in milliseconds from UNIX epoch (or custom epoch).

**Machine Process ID** - or MPID is a unique ID for a machine the server process it is running. A machine may run more than one server process as long as each server process has a unique MPID. All threads on a process share the same MPID.

**Sequence number** - a unique per-millisecond number that is shared between threads on a single process. This number is reset to 0 on the start of a new millisecond.

## Design Constraints
1. Every generated **Snowflake** must be unique among all **process** among all **servers** 
2. No repeat process IDs, all **servers** and their **process** must have unique **process** IDs. Threads within a single **process** must have shared the same process IDs. 
3. No repeat **sequence number**s within the same **process** within the same millisecond
4. The **sequence number** must reset on the edge of each millisecond
5. No use of locks (mutexes, semaphores, or otherwise)
6. All operations on the shared **sequence number** must be atomically synchonized between threads on the same process. 

## Implementation: Basics
My implementation uses a similar version of the Twitter Snowflake format:
```cpp
// Snowflake is stored in the following format:
// |-- 42 bit timestamp --|-- 10 bit MPID --|-- 12 bit sequence number --|
```
Each snowflake is created using the following macro:
```cpp
// u64 machine_id, sequence;
#define MAKE_SNOWFLAKE_FAST(machine_id, sequence)\
  ((machine_id << 54) | (sequence))
```
Where sequence is a compacted timestamp-sequence number value of the following format:
```cpp
// |--- 52 bit compacted timestamp-sequence number ----|
// |-- 42 bit timestamp --|-- 12 bit sequence number --|
```
Using this compacted timestamp-sequence number enables certain sequence rollover detection behaviors that are critical when using atomic operations.

## Implementation: Details 
We first define one ```std::atomic<u64>``` variable in the global namespace to be shared between threads in a single server application process.
```cpp
using u64 = std::uint64_t;
std::atomic<u64> atm_compactSequence;
```
This variable stores the compacted timestamp-sequence number. Note that the 12 bit sequence number is directly before the 42 bit timestamp. This means that if the sequence number overflows, it will auto-increment the timestamp. This makes overflow detection simple by checking if ```atm_compactSequence```'s timestamp is greater than the system timestamp. Furthermore, this behavior automatically reset the sequence number on an overflow.

Using a series of atomic compare exchange operations, we are able to guarantee that sequence numbers are incremented and no repeat ids are generated in the same millisecond. Furthermore, we can guarantee that the sequence number is reset on the start of a new millisecond.

For more information see src/algorithm/Lockfree.h at function ```lockfree::v4b::get```

## Results

... in progress/in development ...

## Takeaways

... in progress/in development ...

## Building
Clone the repo to your machine and pull the requied submodules
```bash
cd path/to/cwd
git clone https://github.com/khelmka25/lock-free-snowflake
cd lock-free-snowflake
git submodule update --init --recursive
```

Create the build folder and navigate to it
```bash
mkdir build
cd build
```

Using cmake and make, build the program (you will need cmake, make (or otherwise), and a C++20 compatible compiler)
```bash
cmake ..
make
```

You now have a testing program for the various algorithms in the repo.
```bash
snowflake-test <arguments>
```

## Testing
After building the program, you may interact and run the tests using any of the following command-line inputs:
```bash
-h          # print help screen and exit
-t <n>      # number of threads to run the tests with
-i <n>      # number of ids to generate per thread
-I <n>      # number of total ids to generate
-a <name>   # name algorithm to run, default: all
-v          # use verbose output 
```