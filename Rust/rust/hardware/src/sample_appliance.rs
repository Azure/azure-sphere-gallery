/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

#[cfg(all(
    azure_sphere_target_hardware = "mt3620_rdb",
    azure_sphere_target_definition = "sample_appliance"
))]
include!("sample_appliance/mt3620_rdb.in");

#[cfg(all(
    azure_sphere_target_hardware = "avnet_mt3620_sk",
    azure_sphere_target_definition = "sample_appliance"
))]
include!("sample_appliance/avnet_mt3620_sk.in");

#[cfg(all(
    azure_sphere_target_hardware = "avnet_mt3620_sk_rev2",
    azure_sphere_target_definition = "sample_appliance"
))]
include!("sample_appliance/avnet_mt3620_sk_rev2.in");

#[cfg(all(
    azure_sphere_target_hardware = "seeed_mt3620_mdb",
    azure_sphere_target_definition = "sample_appliance"
))]
include!("sample_appliance/seeed_mt3620_mdb.in");

#[cfg(all(
    azure_sphere_target_hardware = "usi_mt3620_bt_evb",
    azure_sphere_target_definition = "sample_appliance"
))]
include!("sample_appliance/usi_mt3620_bt_evb.in");
