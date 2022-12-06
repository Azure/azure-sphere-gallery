# Introduction

cargo-azsphere is a cargo extenion that supports Azure Sphere

# Getting Started

`cargo azsphere package` will create an Azure Sphere AppPackage for the app specified
by the current Cargo.toml file using default settings.  To customize, add the following
to your Cargo.toml:

```toml
[package.metadata.azsphere]
app_manifest = "app_manifest.json"
arv = "14"
# list of extra files to package.  Source path first, relative to Cargo.toml, dest file second, relative to package root
extra_files = [
    ["README.md", "files/README.md"],
    ["image.bmp"]
]
```

Where:

- app_manifest allows you to specify an alternate path or filename for the AppManifest file
- arv is the Application Runtime Version
- extra_files is an optional list of files to copy into the AppPackage.  Each entry is an
  array that specifies a source filename, relative to the directory containing Cargo.toml.
  The second entry in the array is optional, the pathname and filename to use as the
  destination in the AppPackage.  If it is omitted, the source name is used as the
  destination.

# Build and Test

Use `cargo build` to build the extension, then ensure it is on your PATH.

# Contribute

This project uses an MIT license.  Please submit a pull request and the maintainers will repsond.
