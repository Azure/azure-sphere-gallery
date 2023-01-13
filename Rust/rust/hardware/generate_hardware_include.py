#!/usr/bin/python3

# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import json
import argparse

# Map "Type" from JSON into a Rust type
typemap = {
    "Gpio": "u32",
    "Pwm": "u32",
    "Adc": "u32",
    "I2cMaster": "u32",
    "SpiMaster": "u32",
    "Uart": "u32",
    "int": "i32"
}

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(
        description='Generate Rust hardware include from JSON')

    arg_parser.add_argument('src', type=str,
                            help='Source JSON to read from')
    arg_parser.add_argument('hw', type=str,
                            help='MT3620.json file')
    arg_parser.add_argument('dst', type=str,
                            help='Destination Rust .in file to write to')

    args = arg_parser.parse_args()

    # Load in mt3620.jon, as the hardware source.
    hw_peripherals = []
    with open(args.hw, 'r') as file:
        j = json.load(file)
        for p in j["Peripherals"]:
            hw_peripherals.append(p["Name"])

    with open(args.src, 'r') as file:
        j = json.load(file)
        with open(args.dst, 'w') as dest:
            dest.write(
                f'/// {j["Metadata"]["Type"]} for {j["Description"]["Name"]}\n')
            for content_line in j["Description"]["MainCoreHeaderFileTopContent"]:
                dest.write(f'{content_line}\n')
            dest.write('\n\n')

            if 'Imports' in j:
                dest.write('use crate::mt3620;\n\n')
                path = j["Imports"][0]["Path"]
                if path != "mt3620.json":
                    dest.write('use crate::azure_sphere_hardware;\n\n')

            for p in j["Peripherals"]:
                dest.write(f'/// {p["Comment"]}\n')

                # Map the JSON type to Rust.  Note that the ADC_ChannelId type in C is uint32_t
                # but the JSON type is "int".  Patch it here so it becomes u32.  Same with
                # PWM_ChannelId.
                rust_type = typemap[p["Type"]]
                if "ADC_CHANNEL" in p["Name"]:
                    rust_type = "u32"
                elif "PWM_CHANNEL" in p["Name"]:
                    rust_type = "u32"

                if 'Mapping' in p:
                    if p["Mapping"] in hw_peripherals:
                        crate = "mt3620"
                    else:
                        crate = "azure_sphere_hardware"

                    dest.write(
                        f'pub const {p["Name"]}:{rust_type} = {crate}::{p["Mapping"]};\n\n')
                else:
                    # Clean up the values a bit
                    value = p["MainCoreHeaderValue"]
                    value = value.replace('(', '')
                    value = value.replace(')', '')
                    value = int(value)

                    dest.write(
                        f'pub const {p["Name"]}: {rust_type}={value};\n\n')
