#!/bin/bash
# --------------------------------------------------------------------------------------------
# Copyright (c) Microsoft Corporation. All rights reserved.
# This file is licensed under the MIT License.
#
# Bash script to Disable/Enable Azure Sphere Classic CLI
# --------------------------------------------------------------------------------------------

ROOT_DIR_NAME="azurespheresdk"
INSTALL_DIR="/opt/"
INSTALL_LOCATION="${INSTALL_DIR}${ROOT_DIR_NAME}"
CLI_V1_PATH="${INSTALL_LOCATION}/Tools"
CLI_V1_HIDDEN_PATH="${INSTALL_LOCATION}/.Tools"
LINKS_DIR="${INSTALL_LOCATION}/Links"
CLI_V1_LINK="${LINKS_DIR}/azsphere_v1"
CLI_V1_HIDDEN_LINK="${LINKS_DIR}/.azsphere_v1"
CLI_V2_LINK="${LINKS_DIR}/azsphere_v2"
AZSPHERE_LINK="${LINKS_DIR}/azsphere"
AZSPHERE_V1_PATH="${LINKS_DIR}/azsphere_v1"
AZSPHERE_V2_PATH="${LINKS_DIR}/azsphere_v2"

LOCAL_Yy="Yy"
LOCAL_Nn="Nn"

echo "Disable/Enable Azure Sphere Classic CLI"

check_running_as_sudo() {
  if [[ "$EUID" -ne 0 ]]; then exit_with_error "This script must be run with root permissions."; fi
}

exit_with_error() {
  # Exit, logging optional param $1 as an error
  echo -e "$1";
  exit 1
}

user_confirm(){
  # Takes up to two parameters:
  # $1 Confirmation question, to be followed by a localised " (Y/N)", repeated until the user answers
  # #2 Optional preamble, not repeated with question. Useful for long confirmation dialogs.
  RESPONSE=0
  if [[ ! -z "$2" ]]; then
    echo -e "$2"
  fi
  while true; do
    read -p "$1 (${LOCAL_Yy::1}/${LOCAL_Nn::1}) " response
    case $response in
      [${LOCAL_Yy}] ) RESPONSE=0; break; ;;
      [${LOCAL_Nn}] ) RESPONSE=1; break; ;;
      * ) echo "${LOCAL_CONFIRM}"; ;;
    esac
  done
  return $RESPONSE
}

# Switch Classic CLI mode to/from enabled/disabled
switch_cliv1_mode() {
# is Classic CLI enabled
if [[ -e "$CLI_V1_PATH" ]]; then
    echo "Azure Sphere Classic CLI is currently enabled"
    echo "Disabling CLIv1"
    # rename Tools to .Tools
    echo "Remove Tools directory"
    mv ${CLI_V1_PATH} ${CLI_V1_HIDDEN_PATH}
    echo "Remove Azure Sphere Classic CLI link"
    mv ${CLI_V1_LINK} ${CLI_V1_HIDDEN_LINK}
    echo "Remove azsphere file"
    rm ${AZSPHERE_LINK}
    echo "Enable CLIv2 for 'azsphere' command"
    ln -s ${AZSPHERE_V2_PATH} "${LINKS_DIR}/azsphere"
else
    echo "Azure Sphere Classic CLI is currently disabled"
    echo "Enabling Azure Sphere Classic CLI"
    echo "Restore Tools directory"
    mv ${CLI_V1_HIDDEN_PATH} ${CLI_V1_PATH}
    echo "Restore Azure Sphere Classic CLI link"
    mv ${CLI_V1_HIDDEN_LINK} ${CLI_V1_LINK}
    echo "remove azsphere file"
    rm ${AZSPHERE_LINK}

    # Prompt - enable Azure Sphere Classic CLI?
    if user_confirm "Use the recommended (new) CLI for 'azsphere'?" "${CONFIGURE_CLI_MESSAGE}"; then
      ln -s ${AZSPHERE_V2_PATH} "${LINKS_DIR}/azsphere"
    else
      ln -s ${AZSPHERE_V1_PATH} "${LINKS_DIR}/azsphere"
    fi
    chmod 755 ${CLI_V1_LINK}
fi
chmod 755 ${AZSPHERE_LINK}
}

check_running_as_sudo
switch_cliv1_mode
