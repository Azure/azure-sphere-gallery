# Crash Dumps Configure Script

This script can be used to get or set crash dump policy by making `GET` or `PATCH` calls to the Azure Sphere API to view or modify the `AllowCrashDumpsCollection` value for one or many device groups.

## Contents

| File/folder | Description |
|-------------|-------------|
| crashdumps_configure.py  | main Python script  |
| LICENSE.txt | License text for this script |
| README.md | This readme file |
| requirements.txt | List of pip requirements (see [Setup](#Setup) section) |


## Prerequisites

- An internet connection
- [`python`](https://www.python.org/downloads/) (3.x) and [`pip`](https://pip.pypa.io/en/stable/installing/)

## Setup

Install the dependencies

```
pip install -r requirements.txt
```

 ## How to use

 ```
 $ python crashdumps_configure.py -h

usage: crashdumps_configure.py [-h] [--get] [--set {on,off}]
                               [--tenantid TENANTID]
                               [--devicegroupid DEVICEGROUPID [DEVICEGROUPID ...]]
                               [--verbose] [--forceinteractive] [--nocache]

Get or set crash dump policy

optional arguments:
  -h, --help            show this help message and exit
  --get                 Get AllowCrashDumpsCollection
  --set {on,off}        Set AllowCrashDumpsCollection
  --tenantid TENANTID, -t TENANTID
                        Tenant id for which to get or set
                        AllowCrashDumpsCollection
  --devicegroupid DEVICEGROUPID [DEVICEGROUPID ...], -dg DEVICEGROUPID [DEVICEGROUPID ...]
                        Device group ids (separated by a space) for which to
                        get or set AllowCrashDumpsCollection
  --verbose, -v         Enable verbose logging
  --forceinteractive    Force interactive auth instead of trying to load a
                        token from the cache
  --nocache             Don't cache an (encrypted) access token in a file in
                        the current directory
```
## Examples

Here are some examples to show how to use the script.

### Get current `AllowCrashDumpsCollection` value

#### Get for all device groups in all tenants

```
python crashdumps_configure.py --get
```

#### Get for all device groups in a tenant

```
python crashdumps_configure.py --get --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
```

#### Get for a single device group in a tenant

```
python crashdumps_configure.py --get --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
```

#### Get for multiple device groups in a tenant

```
python crashdumps_configure.py --get --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz
```

### Enable crash dumps

#### Enable for all device groups in all tenants

```
python crashdumps_configure.py --set on
```

#### Enable for all device groups in a tenant

```
python crashdumps_configure.py --set on --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
```

#### Enable for a single device group in a tenant

```
python crashdumps_configure.py --set on --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
```

#### Enable for multiple device groups in a tenant

```
python crashdumps_configure.py --set on --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz
```

### Disable crash dumps

#### Disable for all device groups in all tenants

```
python crashdumps_configure.py --set off
```

#### Disable for all device groups in a tenant

```
python crashdumps_configure.py --set off --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
```

#### Disable for a single device group in a tenant

```
python crashdumps_configure.py --set off --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy
```

#### Disable for multiple device groups in a tenant

```
python crashdumps_configure.py --set off --tenantid xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx --devicegroupid  yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy zzzzzzzz-zzzz-zzzz-zzzz-zzzzzzzzzzzz
```

## Key concepts

- Note that you must select a user account that has the [Administrator role](https://docs.microsoft.com/en-us/azure-sphere/deployment/add-tenant-users#user-management) in your Azure Sphere tenant to run the `--set` command. Any role will work for the `--get` command.
- A 500 (Internal Server Error) response when updating the `AllowCrashDumpsCollection` field usually indicates a transient error. We recommend that you retry the operation in this case.

## Next steps

- For more information on Azure Sphere crash dumps, see [Configure crash dumps](https://review.docs.microsoft.com/en-us/azure-sphere/deployment/configure-crash-dumps?branch=CEV)
- For more information on how Azure Sphere classifies diagnostic data, see [Overview of diagnostic data types](https://review.docs.microsoft.com/azure-sphere/deployment/diagnostic-data-types?branch=CEV)

## Sample expectations

- By default, this script will store a file named 'token_cache' in your current directory that contains an (encrypted) access token for accessing the Azure Sphere API. This file will be decrypted on future runs to streamline authentication. You can disable this feature by passing that `--nocache` command line argument.

### Expected support for the code

There is no official support guarantee for this code, but we will make a best effort to respond to/address any issues you encounter.

### How to report an issue

If you run into an issue with this script, please [open a GitHub issue against this repo](https://github.com/Azure/azure-sphere-gallery/issues).

## Licenses

For information about the licenses that apply to this script, see [LICENSE.txt](./LICENSE.txt)