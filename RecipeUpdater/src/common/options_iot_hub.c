/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <getopt.h>
#include <stdlib.h>
#include <memory.h>

#include <applibs/log.h>

#include "options.h"
#include "connection_iot_hub.h"

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "The command line arguments for the application shoud be set in app_manifest.json as below:\n"
    "\" CmdArgs \": [\"--Hostname\", \"<azureiothub_hostname>\", \"--CertPath\", \"<cert.crt>\", \"--Endpoint\", \"<API endpoint>\"]\n";

static char *hostname = NULL; // Azure IoT Hub Hostname.
static char *certPath = NULL;
static char *endpoint = NULL;

static ExitCode ValidateUserConfiguration(void);

static Connection_IotHub_Config config = {
    .hubHostname = NULL,
    .certPath = NULL,
    .endpoint = NULL
};

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
ExitCode Options_ParseArgs(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'},
        {.name = "Endpoint", .has_arg = required_argument, .flag = NULL, .val = 'e'},
        {.name = "CertPath", .has_arg = optional_argument, .flag = NULL, .val = 'c'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "h:e:c:", cmdLineOptions, NULL)) != -1) {
        // Check if arguments are missing. Every option requires an argument.
        if (optarg != NULL && optarg[0] == '-') {
            Log_Debug("WARNING: Option %c requires an argument\n", option);
            continue;
        }
        switch (option) {
        case 'h':
            Log_Debug("Hostname: %s\n", optarg);
            hostname = optarg;
            break;
        case 'e':
            Log_Debug("Endpoint: %s\n", optarg);
            endpoint = optarg;
            break;
        case 'c':
            Log_Debug("CertPath: %s\n", optarg);
            certPath = optarg;
            break;
        default:
            // Unknown options are ignored.
            break;
        }
    }

    return ValidateUserConfiguration();
}

void *Options_GetConnectionContext(void)
{
    return (void *)&config;
}

static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (hostname == NULL) {
        validationExitCode = ExitCode_Validate_Hostname;
        Log_Debug(cmdLineArgsUsageText);
    } else if (endpoint == NULL) {
        validationExitCode = ExitCode_Validate_Endpoint;
        Log_Debug(cmdLineArgsUsageText);
    } else {
        Log_Debug("Using Direct Connection: Azure IoT Hub Hostname %s\n", hostname);
        config.hubHostname = hostname;
        config.certPath = certPath;
        config.endpoint = endpoint;
    }

    return validationExitCode;
}
