/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <getopt.h>
#include <stdlib.h>
#include <memory.h>

#include <applibs/log.h>

#include "options.h"

// Usage text for command line arguments in application manifest.
static const char *cmdLineArgsUsageText =
    "The command line arguments for the application shoud be set in app_manifest.json as below:\n"
    "\" CmdArgs \": [\"--Hostname\", \"<your_event_grid_mqtt_hostname>\"]\n";

static char *hostname = NULL; // Azure Event Grid Hostname.
static const char *eventGridPlaceholder = "<your_event_grid_mqtt_hostname>";

static ExitCode ValidateUserConfiguration(void);

/// <summary>
///     Parse the command line arguments given in the application manifest.
/// </summary>
ExitCode Options_ParseArgs(int argc, char *argv[])
{
    int option = 0;
    static const struct option cmdLineOptions[] = {
        {.name = "Hostname", .has_arg = required_argument, .flag = NULL, .val = 'h'},
        {.name = NULL, .has_arg = 0, .flag = NULL, .val = 0}};

    // Loop over all of the options.
    while ((option = getopt_long(argc, argv, "h:", cmdLineOptions, NULL)) != -1) {
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
        default:
            // Unknown options are ignored.
            break;
        }
    }

    return ValidateUserConfiguration();
}

char *Options_GetAzureEventGridHostname(void)
{
    return hostname;
}

static ExitCode ValidateUserConfiguration(void)
{
    ExitCode validationExitCode = ExitCode_Success;

    if (hostname == NULL) {
        validationExitCode = ExitCode_Validate_Hostname;
        Log_Debug(cmdLineArgsUsageText);
    } else if (strcmp(hostname, eventGridPlaceholder) == 0) {
        validationExitCode = ExitCode_Validate_Hostname;
        Log_Debug("Replace \"<your_event_grid_mqtt_hostname>\" with the value of Event Grid MQTT hostname in app_manifest.json.");
    } else {
        Log_Debug("Azure Event Grid Hostname %s\n", hostname);
    }

    return validationExitCode;
}
