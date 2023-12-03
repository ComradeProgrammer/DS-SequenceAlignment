coderen voor het lab van de cursus "Distributed System"

# For developers
## Third-party frameworks and libraries
Sometimes you may want to know about third-party frameworks and libraries used in this project, and sometimes you **may** want to read the official docs or APIs in order to implement some features. Here are the introductions and links to them. When necessary, you can refer to the following materials:

For websocket server, we use crow framework. The official doc is here https://crowcpp.org/master/

For websocket client, we use websocketpp. The repo is https://github.com/zaphoyd/websocketpp and the official doc is https://www.zaphoyd.com/projects/websocketpp/

For json serialization, we use nlohmann::json. The repo (and official guide) is here https://github.com/nlohmann/json

For command line argument parsing, we use google gflag. The repo is https://github.com/gflags/gflags

For unittest, we use googletest. The repo is https://github.com/google/googletest and the official doc is https://google.github.io/googletest/


## Installation guide 
### 1. Preparation
You need to have the following things installed
- make
- cmake
- gcc/g++ or clang/clang++

### 2. Install dependency libraryies
This project requires boost and asio (we don't need to link them because we only need their header-only part).

For macOS, run
```
brew install asio
brew install boost
```

For ubuntu, you can also use apt for these libraries.

### 3. Fetch the code and update the submodule

There are 2 submodules included in this project, so after clone the repo you also need to fetch them。

```
git clone --recursive git@github.com:ComradeProgrammer/DS-SequenceAlignment.git 
```

### 4. Compile and run
There should be 3 executable binaries 
- master
- slave
- bmaster 

In the project folder, run 
```
cmake . -B build   
cmake --build build -j32
```
`-j32 means use 32 parallel threads. You can modify this number according to the harware of your own computer`

You should be able to see the binaries generated in the build folder.

Just run 
` ./build/master --port=8000` 
`./build/bmaster --port=8001 --master_uri=ws://localhost:8000/websocket`
` ./build/slave --master_uri=ws://localhost:8000/websocket --backup_master_uri=ws://localhost:8001/websocket --node_id=slave1` 
to start them

## 5. Unittest
Use`./build/unittest` to run unittests.

*You can also use `ctest --test-dir build`. We also support ctest in CMakeList.txt, but this is not recommended since you cannot see actual details of unit tests*


