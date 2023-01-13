use crate::config::{Config, ExtraMetadataSource};
use crate::error::Error;
use crate::util;
use serde_json::Value;
use std::env;
use std::fs;
use std::io::{self, Write};
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
    debug_mode: bool,
}

#[derive(Debug)]
pub struct CliSetting {
    verbose: bool,
    debug_mode: bool,
    device_opt: Option<String>,
    extra_metadata: Vec<ExtraMetadataSource>,
}

impl CliSetting {
    pub(crate) fn new(args: CliArgs) -> Self {
        Self {
            verbose: args.common.verbose,
            debug_mode: args.debug_mode,
            device_opt: args.device,
            extra_metadata: args.common.extra_metadata(),
        }
    }

    pub fn do_start(self) -> Result<(), Error> {
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

        let (azsphere, azsphere_args) = util::azsphere_tool_path();
        let mut args: Vec<String> = vec![];
        args.extend(azsphere_args);

        let data = fs::read_to_string(package_config.app_manifest)
            .expect("Unable to read app manifest file");
        let app_manifest: serde_json::Value =
            serde_json::from_str(&data).expect("Unable to parse app manifest file");
        let component_id = app_manifest["ComponentId"].to_string();

        args.push("device".to_string());
        args.push("app".to_string());
        args.push("start".to_string());
        args.push("-i".to_string());
        args.push(component_id.replace("\"", ""));
        if let Some(device) = self.device_opt {
            args.push("-d".to_string());
            args.push(device);
        }
        if self.debug_mode {
            args.push("--debug-mode".to_string());
        }
        if self.verbose {
            args.push("-verbose".to_string());
        }
        println!("Starting app");
        if self.verbose {
            println!("Program: {:?} Args {:?}", azsphere, args);
        }
        let output = Command::new(azsphere)
            .args(args)
            .output()
            .expect("Failed to start app");
        if !output.status.success() {
            println!("azsphere command failed:");
            io::stdout().write_all(&output.stdout).unwrap();
            io::stderr().write_all(&output.stderr).unwrap();
            std::process::exit(output.status.code().unwrap());
        }

        Ok(())
    }
}
