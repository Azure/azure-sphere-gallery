Native Applibs Instructions:
    Run the nativeinstall.sh file as sudo
    Restart user session
        You can check that there's now a folder called "/opt/azurespheresdk/NativeLibs"
        You can also check that the environment variable LD_LIBRARY_PATH contains the path "/opt/azurespheresdk/NativeLibs/usr/lib"
    Place the included CMakePresets.json in the root of the project
    Select the 'x86-Debug' configure preset, then build
    (Optional) Modify the .vscode/launch.json to debug in VS Code
        Open the Debug window, pull down launch configuration menu, select Add Configuration, and select C/C++: (gdb) Launch
        Set program to "${workspaceFolder}/out/x86-Debug/<App Name>"
        Save the file and run that configuration

    To uninstall native applibs, run 'nativeinstall.sh -u' as sudo