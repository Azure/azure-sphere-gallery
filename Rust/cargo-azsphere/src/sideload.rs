use crate::config::{Config, ExtraMetadataSource};
use crate::error::Error;
use crate::util;
use serde_json::Value;
use std::env;
use std::io::{self, Write};
use std::path::PathBuf;
use std::process::Command;

#[derive(clap::Parser, Debug)]
pub(crate) struct CliArgs {
    #[clap(flatten)]
    common: super::package::CliArgs,
    /// the device to run the command on when multiple devices are attached.
    /// Values from: azsphere device list-attached. Specify the ID, IP address, or Local Connection ID of the device.
    #[arg(short, long)]
    device: Option<String>,
    /// force the deployment of an image using a Beta API that may no longer be supported.
    #[arg(long)]
    force: bool,
    /// do not automatically start the application after sideload.
    #[arg(short, long)]
    manual_start: bool,
}

#[derive(Debug)]
pub struct CliSetting {
    verbose: bool,
    release_opt: bool,
    device_opt: Option<String>,
    force_opt: bool,
    manual_start_opt: bool,
    extra_metadata: Vec<ExtraMetadataSource>,
}

impl CliSetting {
    pub(crate) fn new(args: CliArgs) -> CliSetting {
        CliSetting {
            verbose: args.common.verbose,
            release_opt: args.common.release,
            device_opt: args.device,
            force_opt: args.force,
            manual_start_opt: args.manual_start,
            extra_metadata: args.common.extra_metadata(),
        }
    }

    // Determine if the host environment is WSL or not
    pub fn do_sideload(self) -> Result<(), Error> {
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
        let manifest_file_dir = env::current_dir()?;
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
        let build_flavor = if self.release_opt { "release" } else { "debug" };
        let target_directory = cargo_metadata["target_directory"]
            .to_string()
            .replace("\"", "");
        let mut target_path = PathBuf::from(target_directory);
        target_path.push("armv7-unknown-linux-musleabihf");
        target_path.push(build_flavor);

        // ${AzureSphereDefaultSDKDir}/Tools_v2/azsphere image-package pack-application --package-directory out/ --destination ${APPNAME}.imagepackage --target-api-set ${ARV}
        let app_package_name = target_path
            .clone()
            .join(package_config.name + ".imagepackage");
        let app_package_name = app_package_name.as_os_str().to_str().unwrap();
        let (azsphere, azsphere_args) = util::azsphere_tool_path();
        let mut args: Vec<String> = vec![];
        args.extend(azsphere_args);
        let translated_app_package_name = if util::is_wsl() {
            if self.verbose {
                println!("calling wslpath");
            }
            let output = Command::new("wslpath")
                .args(["-w", app_package_name])
                .output()
                .expect("wslpath failed");
            if !output.status.success() {
                println!("wslpath command failed:");
                io::stdout().write_all(&output.stdout).unwrap();
                io::stderr().write_all(&output.stderr).unwrap();
                std::process::exit(output.status.code().unwrap());
            }
            let translated = String::from_utf8(output.stdout.clone()).unwrap();
            translated.replace("\n", "")
        } else {
            app_package_name.to_string()
        };

        args.push("device".to_string());
        args.push("sideload".to_string());
        args.push("deploy".to_string());
        args.push("-p".to_string());
        args.push(translated_app_package_name);
        if let Some(device) = self.device_opt {
            args.push("-d".to_string());
            args.push(device);
        }
        if self.force_opt {
            args.push("--force".to_string());
        }
        if self.manual_start_opt {
            args.push("-m".to_string())
        }
        if self.verbose {
            args.push("--verbose".to_string());
        }

        println!("Sideloading {}", app_package_name);
        if self.verbose {
            println!("Program: {:?} Args {:?}", azsphere, args);
        }
        let output = Command::new(azsphere)
            .args(args)
            .output()
            .expect("Failed to sideload");
        if !output.status.success() {
            println!("azsphere command failed:");
            io::stdout().write_all(&output.stdout).unwrap();
            io::stderr().write_all(&output.stderr).unwrap();
            std::process::exit(output.status.code().unwrap());
        }

        Ok(())
    }
}
