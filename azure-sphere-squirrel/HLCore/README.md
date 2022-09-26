# High Level App
This project makes use of the Squirrel-Lang VM (http://www.squirrel-lang.org/), as its higher-level language. Squirrel was originally developed for the gaming industry as an alternative to Lua that provided a famililar, JavaScript-like C syntax in a small package, and that gave greater control over Garbage Collection to meet the stringent real-time performance needs of the industry. These qualities have made it an excellent fit for embedded development.

In the project, additional Squirrel-Lang libraries have been provided for:
- GPIO (wrapping AppLibs/gpio.h).
- Web requests (wrapping CURL for sync and CURL Multi Socket + AppLibs/eventloop.h for async).
- JSON Encode/Decode (implemented in C for performance, using the high-speed JSMN project for Decode).
- PrettyPrint (using JSON Encode to provide human-readable output of Squirrel objects).
- Async task scheduling.

Built-in Squirrel function print() has also been ported to output via AppLibs/log.h Log_Debug functions.

The test framework is a fork of Kiwi Power's Nutkin project, built as a clone of Javascript's Mocha framework, including a feature-complete mocking library. The Squirrel VM will happily run atop Linux/Windows and so it is relativly straight-forward to mock/stub-out Squirrel libraries for CI/CD testing locally on PC or as part of a standard build pipeline.

Squirrel-lang and this project are both written in C++. C++ is not a fully supported language on Azure Sphere because of limitations including no unwind support (no try/catch) and no support for stdlib including the 'new' operator. The code has been modified to work around these limitations.

## Contents
| File/folder | Description |
|-------------|-------------|
| `certs`               | Contains certificate files for use in HTTPS verification |
| `jsmn`                | Submodule of the JSMN JSON Parser Project |
| `nutkin`              | Submodule to fork of the Nutkin Squirrel Test Framework Project |
| `squirrel`            | Submodule to fork (modified for Sphere) of the Squirrel-Lang Project |
| `app_manifest.json`   | The application manifest file |
| `CmakeLists.txt`      | The CMake file contains a set of directives and instructions describing source files and targets |
| `CMakePresets.json`   | The CMake file contains a set of build-presets for different compiler optimisation levels |
| `CmakeSettings.json`  | The CMake setting file |
| `README.md`           | This README file |
| `*.cpp|h`             | Source code |
| `main.nut`            | Squirrel-lang source code demonstrating various APIs|
| `test.nut`            | Squirrel-lang source code demonstrating the Nutkin Test Framework|

## Running the code - main.nut and test.nut
This project is pre-configured to use the `Avnet Azure Sphere MT3620 Starter Kit 2.0` and is intended to be executed on a development device, able to receive debug logs via `GDB`.

Two Squirrel sample source files are provided, `main.nut` and `test.nut`. By default the project will compile and run with `main.nut`, this may be modified to compile and run `test.nut` instead by editing `CmdArgs` within the `app_manifest.json` to point to the appropriate file. `CmdArgs` is a convenient mechanism for passing command-line arguments to your program.

### main.nut
`main.nut` demonstates the following capabilities in order:
1. Printing of strings to debug output.
2. Configuring GPIO and blinky using async tasks.
3. Scheduling of async tasks of various timings and orderings.
4. Some math.
5. JSON decode and PrettyPrint.
6. Async and Sync HTTP PUT requests.
7. REPL, downloading, compiling and executing source-code at the press (and brief hold) of SW2.

To setup `main.nut`, you may need to provide your own HTTP(S) endpoints to receive the PUT requests and to return a string of source-code to the GET request. We've used the service `pipedream.net` as an easy tool for creating endpoints and inspecting requests. You can modify these by providing your own endpoints to the consts `HTTP_PUT_TEST_ENDPOINT` and `HTTP_GET_SOURCE_TEST_ENDPOINT` on lines 1 and 2 respectively. You will also need to allow access to these endpoints through the Azure Sphere firewall by adding them to the `Capabilities->AllowedConnections` in the `app_manifest.json`.

### test.nut
test.nut demonstrates unit testing through the use of Kiwi Power's `Nutkin` test framework. No additional setup is required to run this script.
Unit test examples are to be found at the very end of test.nut and test the output of the `json.encode` and `json.decode` commands.

## Next steps
There's much that could be added to this project:
- Exposing more MT3620 drivers and libraries such as Azure IoT Hub device SDK for C or UART.
- Support for lower-power sleep states, OS update and network notifications.
- Support for running Squirrel on the two Real-Time 200MHz M4 cores.
- A Squirrel .nut file build-system to allow Squirrel source across multiple files.

Contributions are welcome, the project root [README](../README.md) provides more information.