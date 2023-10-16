# Axy - Asynchronous proxy for SDiFI

## Building

### Requirements

- CMake >=3.21
- [Buf](https://buf.build/docs/installation)
- Protobuf (`libprotobuf` and `protoc`)(tested with 3.21.12)
- gRPC++ (and `grpc_cpp_plugin`) >= 1.39.0 (tested with 1.51)
- hiredis >= 0.12.1

On Ubuntu 23.04 these can be installed with:

``` shell
sudo apt install libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev \
                 libhiredis-dev cmake
```

Other dependencies are automatically downloaded and built with CMake's
FetchContent. See [deps.cmake](cmake/deps.cmake) for details.

- [redis++](https://github.com/sewenew/redis-plus-plus)
- [nhlomann_json](https://github.com/nlohmann/json)
- [fmtlib](https://github.com/fmtlib/fmt)
- [CLI11](https://github.com/CLIUtils/CLI11)

### Build with CMake

To build a release build:

``` shell
cmake -Bbuild -H. -DCMAKE_BUILD_TYPE=Release -DPREFER_STATIC=ON -GNinja
cmake --build build
```

This will build all targets. The main one is `build/src/axy/axy` which is the
proxy itself.

### Build Docker image

``` shell
docker build -t axy .
```

## Running

See options for `axy`.

```
$ build/src/axy/axy -h  # or docker run axy -h

Asynchronous/ASR proxy for SDiFI
Usage: build/src/axy/axy [OPTIONS]

Options:
  -h,--help                   Print this help message and exit
  --version                   Display program version information and exit
  --log-level TEXT:{trace,debug,info,warn,error} [info] 
  --listen-address TEXT [localhost:50051] 
  --backend-speech-server-address TEXT [speech.tiro.is:443] 
                              gRPC server that provides the `tiro.speech.v1alpha.Speech` service.
  --backend-speech-server-use-tls
  --redis-address TEXT [tcp://localhost:6379] 
                              The server will write conversation events to streams with keys 'sdifi/conversation/{conv_id}' where {conv_id} is the conversation ID.

```

