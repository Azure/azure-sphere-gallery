/* Copyright (c) Microsoft Corporation. All rights reserved.
Licensed under the MIT License. */

#[cfg(azure_sphere_target_hardware = "mt3620_rdb")]
include!("mt3620_rdb.in");

#[cfg(azure_sphere_target_hardware = "ailink_wfm620rsc1")]
include!("ailink_wfm620rsc1.in");

#[cfg(azure_sphere_target_hardware = "avnet_aesms_mt3620")]
include!("avnet_aesms_mt3620.in");

#[cfg(azure_sphere_target_hardware = "avnet_aesms_mt3620_rev2")]
include!("avnet_aesms_mt3620_rev2.in");

#[cfg(azure_sphere_target_hardware = "avnet_mt3620_sk")]
include!("avnet_mt3620_sk.in");

#[cfg(azure_sphere_target_hardware = "avnet_mt3620_sk_rev2")]
include!("avnet_mt3620_sk_rev2.in");

#[cfg(azure_sphere_target_hardware = "seeed_mt3620_mdb")]
include!("seeed_mt3620_mdb.in");

#[cfg(azure_sphere_target_hardware = "usi_mt3620_bt_combo")]
include!("usi_mt3620_bt_combo.in");

#[cfg(azure_sphere_target_hardware = "usi_mt3620_bt_evb")]
include!("usi_mt3620_bt_evb.in");
