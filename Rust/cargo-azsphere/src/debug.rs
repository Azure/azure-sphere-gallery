use crate::config::{Config, ExtraMetadataSource};
use crate::error::Error;
use crate::util;
use serde_json::Value;
use std::env;
use std::fs;
use std::io::Read;
use std::io::{self, Write};
use std::net::TcpStream;
use std::path::PathBuf;
use std::process::Command;
use std::str::from_utf8;
use std::thread;
extern crate ctrlc;

#[derive(clap::Parser, Debug)]
pub(crate) struct CliArgs {
    #[clap(flatten)]
    common: super::package::CliArgs,
    /// the device to run the command on when multiple devices are attached.
    /// Values from: azsphere device list-attached. Specify the ID, IP address, or Local Connection ID of the device.
    #[arg(short, long)]
    device: Option<String>,
    /// Use VSCode to debug instead of console gdb
    #[arg(long)]
    use_vs_code: bool,
}

#[derive(Debug)]
pub struct CliSetting {
    verbose: bool,
    release: bool,
    use_vs_code: bool,
    device_opt: Option<String>,
    extra_metadata: Vec<ExtraMetadataSource>,
}

impl CliSetting {
    pub(crate) fn new(args: CliArgs) -> Self {
        Self {
            verbose: args.common.verbose,
            release: args.common.release,
            use_vs_code: args.use_vs_code,
            device_opt: args.device,
            extra_metadata: args.common.extra_metadata(),
        }
    }

    pub fn do_debug(self) -> Result<(), Error> {
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

        let device_ip = if let Some(device) = self.device_opt {
            args.push("-d".to_string());
            args.push(device.clone());
            device
        } else {
            "192.168.35.2".to_string()
        };
        args.push("--debug-mode".to_string());
        if self.verbose {
            args.push("--verbose".to_string());
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

        println!("Connecting to {}:2342...\n", device_ip);

        // Now that the application has been started, connect to the device's port 2342 to receive its output stream
        match TcpStream::connect((device_ip.clone(), 2342)) {
            Ok(mut stream) => {
                thread::spawn(move || loop {
                    let mut data = [0 as u8; 2048];
                    match stream.read(&mut data) {
                        Ok(_) => match from_utf8(&data) {
                            Ok(text) => {
                                let text = text.trim_matches(char::from(0));
                                if text.len() > 0 {
                                    print!("{}", text)
                                }
                            }
                            Err(e) => {
                                println!("UTF8 conversion error: {:?}\n", e)
                            }
                        },
                        Err(e) => {
                            println!("Failed to read from the device on port 2342: {:?}", e);
                            std::process::exit(1);
                        }
                    }
                });
            }
            Err(e) => {
                println!("Failed to connect to the device on port 2342: {:?}", e);
                std::process::exit(1);
            }
        }

        if self.use_vs_code {
            println!("Switch to Visual Studio Code and hit F5 to begin debugging\n");
            println!("Hit enter when finished debugging.\n");
            let mut input = String::new();
            let _ = io::stdin().read_line(&mut input);
        } else {
            if self.verbose {
                println!("Starting debugger\n");
            }

            let build_flavor = if self.release { "release" } else { "debug" };
            let target_directory = cargo_metadata["target_directory"]
                .to_string()
                .replace("\"", "");
            let mut target_path = PathBuf::from(target_directory);
            target_path.push("armv7-unknown-linux-musleabihf");
            target_path.push(build_flavor);
            let source_program = target_path.join(&package_config.name);

            let target_remote = format!("target remote {}:2345", device_ip);

            // ${AzureSphereDefaultSDKDir}/Sysroots/${ARV}/tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-gdb <args>
            let sdk_path = PathBuf::from(std::env::var("AzureSphereDefaultSDKDir").unwrap());
            let gdb_command = sdk_path.join("Sysroots").join(&package_config.arv).join("tools/sysroots/x86_64-pokysdk-linux/usr/bin/arm-poky-linux-musleabi/arm-poky-linux-musleabi-gdb");
            if self.verbose {
                println!("Running arm-poky-linux-musleabi-gdb");
            }
            let mut command = Command::new(gdb_command);
            command
                .arg("-ex")
                .arg(target_remote)
                .arg(source_program.as_os_str());
            if self.verbose {
                println!(
                    "Launching {:?} {:?}\n",
                    command.get_program(),
                    command.get_args(),
                );
            }

            // Set an empty Ctrl+C handler, so that Ctrl+C is passed to gdb and handled there.  Otherwise,
            // the cargo-azsphere process is killed.
            ctrlc::set_handler(move || {}).expect("Error setting Ctrl-C handler");

            // Launch gdb async, so that its stdout and stdin are unmodified, and wait for it to complete.
            let mut child = command.spawn().expect("Failed to execute gdb");
            let _ = child.wait().expect("Failed to run");
            if self.verbose {
                println!("Back from gdb\n");
            }
        }

        Ok(())
    }
}
