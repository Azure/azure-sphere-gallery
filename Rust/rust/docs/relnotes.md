# Release Notes

## Known Issues

1. No support for [DAA](https://azure.microsoft.com/en-us/resources/azure-sphere-device-authentication-and-attestation-service/en-us/#:~:text=The%20Device%20Authentication%20and%20Attestation%20%28DAA%29%20service%20is,that%20they%20are%20running%20a%20trusted%20code%20base.) mutual authentication with Azure IoT.  The unofficial [Azure SDK for Rust](https://github.com/Azure/azure-sdk-for-rust) compiles and runs on Azure Sphere, but does not yet have DAA integration.  There is DAA integration with `curl-rust` via the applibs `deviceauth_curl` module.

1. Not all samples from the Azure Sphere C SDK are ported.

1. Application binary sizes are large.  `cargo bloat` can help understand code size.  Work-in-progress in Rust nightly may dramatically reduce our binary size in the future.  See this article [Optimize libstd with build-std](https://github.com/johnthagen/min-sized-rust#optimize-libstd-with-build-std).

1. `cargo doc` will generate two warnings about `could not parse code block as Rust code` from `/target/armv7-unknown-linux-musleabihf/debug/build/azure-sphere-sys-c6e5bb0ea1db5076/out/storage.rs`.
