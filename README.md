## OvenWebRTCTester
 - The purpose is to measure the WebRTC performance of OvenMediaEngine.

## Features
- WebRTC Streaming

## Supported Platforms
- Ubuntu 18
- Windows 10
We will support the following platforms in the future:
- (Centos 7)

## Quick Start
### Prerequisites
- Ubuntu
  - Install packages
  ```
  sudo apt install build-essential nasm autoconf libtool zlib1g-dev libssl-dev libboost-all-dev pkg-config tclsh cmake
  ```


  ### Build
  You can build source with the following command
  ```
  $ git clone "https://github.com/AirenSoft/OvenWebRTCTester.git"
  $ cd OvenWebRTCTester
  $ make && make install
  $ cd bin
  ```

  ### Try it
  ```
  $ cd [SOURCE_PATH]/bin

  [Edit OvenWebRTCTester.conf File]
  ex)
  signalling_address                      192.168.0.160
  signalling_port                         3333
  signalling_app                          app
  signalling_stream                       stream_o
  streaming_start_count                   10
  streaming_create_interval               5
  streaming_create_count                  50
  streaming_max_count                     500

  log_file_path                           logs

  $ ./OvenStreamTester
