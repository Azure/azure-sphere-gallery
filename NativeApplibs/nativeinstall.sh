#!/bin/bash
show_usage() {
  echo "Usage: nativeinstall.sh [-u]"
  echo "       nativeinstall.sh -h"
  echo "Install the Azure Sphere SDK native applibs"
  echo "Params:"
  echo "-u: Uninstall the Azure Sphere SDK native applibs"
  echo "-h: Show help"
  echo "Requires root permissions"
  echo "Depends on libwolfssl24"
}

if [[ "$EUID" -ne 0 ]]; then echo "ERROR: This script must be run with root permissions."; exit 1; fi
uninstall=0
while getopts ":uh" opt; do
  case "$opt" in 
    u)
      uninstall=1
      ;;
    h)
      show_usage
      exit 0
      ;;
    *)
      echo "ERROR: Unsupported option"
      show_usage
      exit 1
      ;;
  esac
done

if [[ $uninstall -eq 0 ]]; then
  dpkg -s libwolfssl24 > /dev/null 2>&1

  if [[ $? -ne 0 ]]; then
    echo "ERROR: missing dependency: libwolfssl24"
    echo "Please run command 'sudo apt-get install -y libwolfssl24'"
    exit 1
  fi

  echo "Installing Azure Sphere native libs..."
  mkdir -p /opt/azurespheresdk/NativeLibs/usr/lib
  mkdir -p /opt/azurespheresdk/NativeLibs/usr/include

  cp ./AzureSphereNativeToolchain.cmake /opt/azurespheresdk/NativeLibs
  cp ./lib/libapplibs.so /opt/azurespheresdk/NativeLibs/usr/lib/libapplibs.so.0.1
  cp ./lib/libc++runtime.so /opt/azurespheresdk/NativeLibs/usr/lib/libc++runtime.so.1

  ln -s /opt/azurespheresdk/NativeLibs/usr/lib/libapplibs.so.0.1 /opt/azurespheresdk/NativeLibs/usr/lib/libapplibs.so.0
  ln -s /opt/azurespheresdk/NativeLibs/usr/lib/libapplibs.so.0 /opt/azurespheresdk/NativeLibs/usr/lib/libapplibs.so
  ln -s /opt/azurespheresdk/NativeLibs/usr/lib/libc++runtime.so.1 /opt/azurespheresdk/NativeLibs/usr/lib/libc++runtime.so

  cp -r ./include/* /opt/azurespheresdk/NativeLibs/usr/include

  echo "export LD_LIBRARY_PATH=\"\$LD_LIBRARY_PATH:/opt/azurespheresdk/NativeLibs/usr/lib\"" > /etc/profile.d/azure-sphere-sdk-native.sh
else
  echo "Uninstalling Azure Sphere native libs..."
  rm -r /opt/azurespheresdk/NativeLibs
  rm /etc/profile.d/azure-sphere-sdk-native.sh
fi

echo "Success. You will need to restart your user session for changes to take effect"
