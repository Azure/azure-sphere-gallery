use std::path::{Path, PathBuf};

pub fn is_wsl() -> bool {
    Path::new("/proc/sys/fs/binfmt_misc/WSLInterop").exists()
}

pub fn azsphere_tool_path() -> (PathBuf, Vec<String>) {
    let mut args: Vec<String> = vec![];
    let azsphere = if is_wsl() {
        // Use the Windows azsphere.exe, found on the path, as it can communicate with the device via SLIP etc.
        args.push("-IBm".to_string());
        args.push("azure.cli".to_string());
        PathBuf::from("/mnt/c/Program Files (x86)/Microsoft Azure Sphere SDK/Tools_v2/python.exe")
    } else {
        let sdk_path = PathBuf::from(std::env::var("AzureSphereDefaultSDKDir").unwrap());
        sdk_path.join("Tools_v2/azsphere")
    };
    (azsphere, args)
}
