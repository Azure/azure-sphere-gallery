# Quickstart: Deploy a Rust sample to an Azure Sphere DevKit

This quickstart will walk you through getting an Azure Sphere sample written in Rust deployed to an Azure Sphere device using tooling familiar to Rust developers.

## Prerequisites

- You must have access to an Azure Sphere development kit.
- You must follow the instructions on the [Azure Sphere documentation](https://docs.microsoft.com/en-us/azure-sphere/install/install-sdk-linux?pivots=vs-code-linux) page to set up your Linux machine properly.

## Check Installation

1. Make sure azsphere CLIv2 is the default.  The Classic CLI is not supported.  See [Migrate from classic CLI to Azure Sphere CLI](https://learn.microsoft.com/en-us/azure-sphere/reference/classic-cli-migration?tabs=cliv2beta).
1. Check that the SDK was properly installed by running `azsphere`. You should see the help instructions.
    - On Windows with WSL, install both the Windows and Linux SDKs.  The Linux azsphere is used for most operations, but sideload will invoke Windows azsphere.
1. Ensure that the environment has been properly initialized by running `echo $AzureSphereDefaultSDKDir`. You should see a file path printed as output. Note: this may require starting a new shell session before it works properly.

## Setup

1. Install [`rustup`](https://rustup.rs), the Rust toolchain manager. This will give you the Rust compiler (rustc) and the Rust build tool/dependency manager (cargo).

1. Install the proper target for the Azure Sphere device:

    > rustup target add armv7-unknown-linux-musleabihf

1. Make sure that your environment has a armv7 cross compiler:

    If running on Ubuntu:

    > sudo apt install llvm libc6-dev-i386 libclang-dev patchelf

    If running on Arch Linux:

    > sudo pacman -S llvm clang patchelf

      Also on Arch Linux, needed (AUR, build with makepkg -si, do not delete sources.): <https://aur.archlinux.org/packages/azure-sphere-sdk/>

4. Install the `cargo azsphere` helper tool.  From the top-level Rust/ directory:

    > cargo install --path cargo-azsphere/

## Build, Debug and Deploy from Command Line

1. Navigate to one of the examples in the `samples` directory:

```bash
cd rust/samples/HelloWorld/hello_world_high_level_app
```

1. Compile the binary:

    > cargo build

1. Build the image package:

    > cargo azsphere package

1. Deploy the image package onto the device.

    > cargo azsphere sideload -m

1. To debug your application, run the following command

    > cargo azsphere debug

    Note that the debugger will attach at the first instruction inside the app. Use
    these commands to run a little further, into the start of `main`:

    ```text
      break main
      cont
    ```

1. After the breakpoint in main, the expected output is the following in a loop:

    ```text
      Breakpoint 1, 0x00014a58 in main ()
      (gdb) cont
      Continuing.
      Starting Rust Hello World application...
    ```

    The device's led light should also blink on and off in 1 second cycles.

## Build, Debug and Deploy using Visual Studio Code

1. Navigate to one of the examples in the `samples` directory from the command line:

```bash
cd rust/samples/HelloWorld/hello_world_high_level_app
```

1. Open Visual Studio Code:

```bash
code .
```

1. Press `F5` to build, deploy and run the application. If you set any breakpoints before pressing `F5`, you will be able to debug the code.

## Viewing documentation

From the top-level rust/ directory, run `cargo doc --open` to compile the documentation and open in a web browser.
The 'crate azure-sphere' contains the Azure Sphere AppLibs code.  The 'crate hardware' contains the hardware abstraction that enables targetting multiple devkits.  Use `cargo doc --open -p azure-sphere` to jump to the azure-sphere crate documentation.

## Tips and Tricks

### Targeting Different Devkits

At the root, .cargo/config specifies `AZURE_SPHERE_TARGET_HARDWARE` and `AZURE_SPHERE_TARGET_DEFINITION`.  These are the equivalent of the Azure Sphere C SDK's CMakeLists.txt TARGET_HARDWARE and TARGET_DEFINTION settings:

```
set(TARGET_HARDWARE "mt3620_rdb")
set(TARGET_DEFINITION "sample_appliance.json")
```

### Targeting a Different SysRoot/ARV

At the root, .cargo/config specifies `AZURE_SPHERE_ARV` and a path to the native linker to invoke.  Edit the value in both locations in order to change to a different ARV.

### Debugging

A handy tool for debugging Rust applications is to set the RUST_BACKTRACE environment variable before running your program.  Azure Sphere doesn't have the concept of environment variables as part of the application.  So there are some workarounds:

1. At the start of your program, set the variable programmatically:

```rust
env::set_var("RUST_BACKTRACE", "full");
```

This will give you a callstack, but with addresses only... no symbol names.  This is because Azure Sphere programs have symbols stripped in order to reduce the on-device program size.  You'll see something like this:

```text
stack backtrace:
   0:    0x2f098 - <unknown>
   1:    0x570e8 - <unknown>
   2:    0x2ce24 - <unknown>
   3:    0x307f0 - <unknown>
   4:    0x30350 - <unknown>
   5:    0x30ffc - <unknown>
   6:    0x30d48 - <unknown>
   7:    0x2f5c0 - <unknown>
   8:    0x30af8 - <unknown>
   9:    0x14280 - <unknown>
  10:    0x142f8 - <unknown>
  11:    0x15830 - <unknown>
```

You can use gdb's `info symbol` command to look up those addresses, like this:

```text
(gdb) info symbol 0x2f098
<std::sys_common::backtrace::_print::DisplayBacktrace as core::fmt::Display>::fmt + 336 in section .text of /home/barry/azrust/target/armv7-unknown-linux-musleabihf/debug/hello_world_high_level_app
```

2. Set a breakpoint on `rust_begin_unwind`.  This will cause gdb to break at the beginning of a panic, before it unwinds the stack.

```text
Breakpoint 1, rust_begin_unwind () at library/std/src/panicking.rs:582
582     library/std/src/panicking.rs: No such file or directory.
(gdb) bt
#0  rust_begin_unwind () at library/std/src/panicking.rs:582
#1  0x00014280 in core::panicking::panic_fmt () at library/core/src/panicking.rs:142
#2  0x000142f8 in core::result::unwrap_failed () at library/core/src/result.rs:1785
#3  0x00015830 in core::result::Result<T,E>::unwrap (self=...)
    at /rustc/897e37553bba8b42751c67658967889d11ecd120/library/core/src/result.rs:1107
#4  0x00015300 in azure_sphere::applibs::log::log_debug (message=...) at azure-sphere/src/applibs/log.rs:51
#5  0x000146dc in hello_world_high_level_app::actual_main ()
    at samples/HelloWorld/hello_world_high_level_app/src/main.rs:19
#6  0x0001490c in hello_world_high_level_app::main () at samples/HelloWorld/hello_world_high_level_app/src/main.rs:37
(gdb) info symbol 0x2f098
<std::sys_common::backtrace::_print::DisplayBacktrace as core::fmt::Display>::fmt + 336 in section .text of /home/barry/azrust/target/armv7-unknown-linux-musleabihf/debug/hello_world_high_level_app
```
