use crate::config::{Config, ExtraMetadataSource};
use crate::error::{self, ConfigError};
use anyhow::Context;
use serde_json::Value;
use std::env;
use std::fs;
use std::io::{self, Write};
use std::path::PathBuf;
use std::process::Command;

#[derive(clap::Parser, Debug)]
#[group(skip)]
pub(crate) struct CliArgs {
    /// set a package name of the workspace
    #[arg(short, long)]
    pub(crate) package: Option<String>,
    /// build artifacts in release mode, with optimizations [DEFAULT = false]
    #[arg(short, long)]
    pub(crate) release: bool,
    /// overwrite metadata with TOML text.
    #[arg(short, long)]
    set_metadata: Vec<String>,
    /// overwrite metadata with TOML file. If suffixed with "#dotted.key", load "dotted.key" table instead of the root table.
    #[arg(long)]
    metadata_overwrite: Vec<String>,
    /// shortcut to --metadata-overwrite=path/to/Cargo.toml#package.metadata.generate-rpm.variants.VARIANT
    #[arg(long)]
    variant: Option<String>,
    /// enable verbose logging
    #[arg(short)]
    pub(crate) verbose: bool,
}

impl CliArgs {
    pub(crate) fn extra_metadata(&self) -> Vec<ExtraMetadataSource> {
        if self.verbose {
            println!("Collecting extra_metadata");
        }

        let mut extra_metadata = self
            .metadata_overwrite
            .iter()
            .enumerate()
            .map(|(i, v)| {
                let (file, branch) = match v.split_once("#") {
                    None => (PathBuf::from(v), None),
                    Some((file, branch)) => (PathBuf::from(file), Some(branch.to_string())),
                };
                (i, ExtraMetadataSource::File(file, branch))
            })
            .chain(
                self.set_metadata
                    .iter()
                    .enumerate()
                    .map(|(i, v)| (i, ExtraMetadataSource::Text(v.clone()))),
            )
            .chain(self.variant.iter().enumerate().map(|(i, v)| {
                let file = match &self.package {
                    None => PathBuf::from("Cargo.toml"),
                    Some(package) => PathBuf::from(package).join("Cargo.toml"),
                };
                let branch = format!("package.metadata.generate-rpm.variants.{v}");
                (i, ExtraMetadataSource::File(file, Some(branch)))
            }))
            .collect::<Vec<_>>();
        extra_metadata.sort_by_key(|(i, _)| *i);
        extra_metadata.into_iter().map(|(_, v)| v).collect()
    }
}

#[derive(Debug)]
pub struct CliSetting {
    release: bool,
    verbose: bool,
    extra_metadata: Vec<ExtraMetadataSource>,
}

impl CliSetting {
    pub(crate) fn new(args: CliArgs) -> Self {
        let extra_metadata = args.extra_metadata();

        Self {
            release: args.release,
            verbose: args.verbose,
            extra_metadata,
        }
    }

    pub fn build_app_package(self) -> anyhow::Result<()> {
        if self.verbose {
            println!("Invoking 'cargo metadata'");
        }
        let output = Command::new("cargo")
            .arg("metadata")
            .arg("--format-version=1")
            .output()
            .expect("Failed to run 'cargo metadata'");
        if self.verbose {
            println!("Parsing metadata");
        }
        let cargo_metadata: Value = serde_json::from_slice(output.stdout.as_slice()).unwrap();

        if self.verbose {
            println!("Finding package config");
        }
        let manifest_file_dir = env::current_dir().context("failed to get manifest file")?;
        let manifest_file_path = manifest_file_dir.join("Cargo.toml");
        let config = Config::new(&manifest_file_path, self.extra_metadata.as_slice())?;
        let package_config = config.package_config(
            cargo_metadata["workspace_root"]
                .to_string()
                .replace("\"", ""),
            self.verbose,
        )?;

        // cargo_metadata["target_directory"] + "armv7-unknown-linux-musleabihf" + "debug" + crate_name is the executable file
        if self.verbose {
            println!("Determining target_path");
        }
        let build_flavor = if self.release { "release" } else { "debug" };
        let target_directory = cargo_metadata["target_directory"]
            .to_string()
            .replace("\"", "");
        let mut target_path = PathBuf::from(target_directory);
        target_path.push("armv7-unknown-linux-musleabihf");
        target_path.push(build_flavor);

        let source_program = target_path.join(&package_config.name);

        let out_dir = target_path.join("out");
        let dest_dir = out_dir.join(&package_config.name);
        if self.verbose {
            println!("Staging files to {}", dest_dir.display());
        }
        let _ = fs::remove_dir_all(&dest_dir); // Remove the previous contents.  Ignore failures, which can happen if the dir doesn't exist.
        fs::create_dir_all(&dest_dir)?; // Create a clean directory

        let dest_bin_dir = dest_dir.join("bin");
        fs::create_dir_all(&dest_bin_dir)?;

        // cp app_manifest.json out/
        let dest_app_manifest = dest_dir.join("app_manifest.json");
        if self.verbose {
            println!("Copy app manifest {}", &package_config.app_manifest);
        }
        fs::copy(&package_config.app_manifest, &dest_app_manifest)
            .context("failed to copy app manifest")?;

        // cp ../target/armv7-unknown-linux-musleabihf/debug/${APPNAME} out/bin
        let dest_program = dest_bin_dir.join(&package_config.name);
        if self.verbose {
            println!(
                "Copying app executable from {} to {}",
                source_program.display(),
                dest_program.display()
            );
        }
        if !source_program.exists() {
            anyhow::bail!(
                "executable `{}` does not exist. Are you sure you compiled it with `cargo build`?",
                source_program.display()
            );
        }
        fs::copy(&source_program, &dest_program).with_context(|| {
            format!(
                "failed to copy executable from {} to {}",
                source_program.display(),
                dest_program.display()
            )
        })?;

        // patchelf --set-interpreter /lib/ld-musl-armhf.so.1 out/bin/${APPNAME}
        if self.verbose {
            println!("Running patchelf");
        }
        let output = Command::new("patchelf")
            .arg("--set-interpreter")
            .arg("/lib/ld-musl-armhf.so.1")
            .arg(dest_program.as_os_str())
            .output()
            .expect("Failed to execute patchelf");
        if !output.status.success() {
            println!("patchelf command failed:");
            io::stdout().write_all(&output.stdout).unwrap();
            io::stderr().write_all(&output.stderr).unwrap();
            std::process::exit(output.status.code().unwrap());
        }

        // ${AzureSphereDefaultSDKDir}/Sysroots/${ARV}/tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-strip --strip-debug --strip-unneeded out/bin/${APPNAME}
        let sdk_path = PathBuf::from(std::env::var("AzureSphereDefaultSDKDir").unwrap());
        let strip_command = sdk_path.join("Sysroots").join(&package_config.arv).join("tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-strip");
        if self.verbose {
            println!("Running arm-poky-linux-musleabi-strip");
        }
        let output = Command::new(strip_command)
            .arg("--strip-debug")
            .arg("--strip-unneeded")
            .arg(dest_program.as_os_str())
            .output()
            .expect("Failed to execute strip");
        if !output.status.success() {
            println!("patchelf command failed:");
            io::stdout().write_all(&output.stdout).unwrap();
            io::stderr().write_all(&output.stderr).unwrap();
            std::process::exit(output.status.code().unwrap());
        }

        // Copy extra files
        if let Some(extra_files) = package_config.extra_files {
            for extra_file in extra_files {
                let extra_file = extra_file.as_array().ok_or(ConfigError::WrongType(
                    "extra_files entry".to_string(),
                    "array",
                ))?;
                if extra_file.len() < 1 || extra_file.len() > 2 {
                    return Err(error::Error::Config(ConfigError::MalFormedArray).into());
                }
                let source_file = extra_file[0].as_str().ok_or(ConfigError::WrongType(
                    "extra_files entry source filename".to_string(),
                    "string",
                ))?;
                let destination_file = if extra_file.len() == 1 {
                    source_file
                } else {
                    extra_file[1].as_str().ok_or(ConfigError::WrongType(
                        "extra_files entry destination filename".to_string(),
                        "string",
                    ))?
                };

                let source_file_path = manifest_file_dir.join(source_file);
                let destination_file_path = dest_dir.join(destination_file);
                if self.verbose {
                    println!(
                        "Copying extra_file {} => {} ",
                        source_file, destination_file
                    );
                }
                let destination_file_directory = destination_file_path.parent().unwrap();
                fs::create_dir_all(destination_file_directory)?;
                fs::copy(source_file_path, destination_file_path)?;
            }
        }

        let target_definition_file =
            if let Some(target_definition) = &package_config.target_definition {
                let t = target_definition.to_owned() + ".json";
                Some(t)
            } else {
                None
            };

        // ${AzureSphereDefaultSDKDir}/Tools_v2/azsphere image-package pack-application --package-directory out/ --destination ${APPNAME}.imagepackage --target-api-set ${ARV}
        let azsphere = sdk_path.join("Tools_v2/azsphere");
        let app_package_name = target_path.join(package_config.name.clone() + ".imagepackage");
        let mut args = vec![
            "image-package",
            "pack-application",
            "--package-directory",
            dest_dir.as_os_str().to_str().unwrap(),
            "--destination",
            app_package_name.as_os_str().to_str().unwrap(),
            "--target-api-set",
            &package_config.arv,
        ];

        let app_package = cargo_metadata["packages"]
            .as_array()
            .unwrap()
            .into_iter()
            .find(|&x| x["name"] == package_config.name);

        let hw_defs_path = if let Some(target_hardware) = &package_config.target_hardware {
            if self.verbose {
                println!("target_hardware {:?}\n", target_hardware);
                println!("app_package: {:?}\n", app_package);
            }
            if let Some(app_package) = app_package {
                let hardware = app_package["dependencies"]
                    .as_array()
                    .unwrap()
                    .into_iter()
                    .find(|&x| x["name"] == "hardware");
                if let Some(hardware) = hardware {
                    let path = &hardware["path"].to_string().replace("\"", "");
                    Some(
                        PathBuf::from(path)
                            .join("HardwareDefinitions")
                            .join(target_hardware),
                    )
                } else {
                    None
                }
            } else {
                None
            }
        } else {
            None
        };

        let sdk_hw_definitions = sdk_path.join("HardwareDefinitions");
        if let Some(hw_defs_path) = &hw_defs_path {
            if let Some(target_definition_file) = &target_definition_file {
                args.push("--target-definition-filename");
                args.push(target_definition_file.as_str());
            }

            if package_config.target_definition.is_some() {
                args.push("--hardware-definitions");
                args.push(hw_defs_path.as_os_str().to_str().unwrap());
                args.push(sdk_hw_definitions.as_os_str().to_str().unwrap());
            }
        }
        if self.verbose {
            args.push("--verbose");
            println!("Args: {:?}\n", args);
        }
        println!(
            "Generating {}",
            app_package_name.as_os_str().to_str().unwrap()
        );
        let output = Command::new(azsphere)
            .args(args)
            .output()
            .expect("Failed to create app package");
        if !output.status.success() {
            println!("azsphere command failed:");
            io::stdout().write_all(&output.stdout).unwrap();
            io::stderr().write_all(&output.stderr).unwrap();
            std::process::exit(output.status.code().unwrap());
        }

        Ok(())
    }
}
