use anyhow::Context;
use clap::Parser;

use crate::error::Error;

mod config;
mod debug;
mod error;
mod package;
mod sideload;
mod start;
mod util;

#[derive(Parser, Debug)]
#[command(version, about)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(clap::Subcommand, Debug)]
enum Command {
    /// Generate an app package.  Customize via [package.metadata.azsphere] in Cargo.toml
    Package(package::CliArgs),
    /// Sideload an app package
    Sideload(sideload::CliArgs),
    /// Start a sideloaded program
    Start(start::CliArgs),
    /// Debug a program
    Debug(debug::CliArgs),
}

fn main() {
    run().unwrap_or_else(|err| {
        eprintln!("❌: {}", err);
        for e in err.chain().skip(1) {
            eprintln!("\t{e}");
        }
        std::process::exit(1);
    });
}

fn run() -> anyhow::Result<()> {
    let mut args = std::env::args().peekable();

    // Attempt to detect when running as a Cargo subcommand
    let cli = if args
        .peek()
        .map(|s| s.contains("azsphere"))
        .unwrap_or_default()
    {
        Cli::parse_from(args.skip(1))
    } else {
        Cli::parse_from(args)
    };
    match cli.command {
        Command::Debug(args) => {
            let settings = debug::CliSetting::new(args);
            settings.do_debug().context("error debugging")?;
        }
        Command::Package(args) => {
            let settings = package::CliSetting::new(args);
            settings
                .build_app_package()
                .context("error building app package")?;
        }
        Command::Sideload(args) => {
            let setting = sideload::CliSetting::new(args);
            setting.do_sideload().context("error sideloading app")?;
        }
        Command::Start(args) => {
            let setting = start::CliSetting::new(args);
            setting.do_start().context("error starting app")?;
        }
    };
    Ok(())
}
