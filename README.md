coderen voor het lab van de cursus "Distributed System"

# For developers
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
- bmaster (Haven't added. T.B.D)

In the project folder, run 
```
cmake . -B build   
cmake --build build -j32
```
`-j32 means use 32 parallel threads. You can modify this number according to the harware of your own computer`

You should be able to see the binaries generated in the build folder.

Just run `./build/master` `./build/slave` to start them


