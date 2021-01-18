# Copyright (c) Microsoft Corporation.
# Licensed under the MIT License.

import argparse
import atexit
import base64
import json
import logging
import msal
import os
import re
import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry
import time
from typing import List

# CONSTANTS -- DO NOT EDIT ---
client_id="0b1c8f7e-28d2-4378-97e2-7d7d63f7c87f"
tenant_id="7d71c83c-ccdf-45b7-b3c9-9c41b94406d9"
scope="https://sphere.azure.net/api/user_impersonation"
authority = "https://login.microsoftonline.com/"+tenant_id
azure_sphere_api_base_url="https://prod.core.sphere.azure.net/v2"

# Name of file to save access token for future use (feel free to edit)
TOKEN_SAVE_FILENAME="token_cache"
# The keyring details to securely store access token
keyring_namespace = 'azuresphere-crashdumps-configure'
keyring_username = 'accessToken'

class AzureSphereAPIClient:

    class AzureSphereAPIClientException(Exception):
        pass

    def __init__(self, forceinteractive:bool, nocache:bool):
        self.forceinteractive = forceinteractive
        self.nocache = nocache
        if not self.nocache:
            self.cache = msal.SerializableTokenCache()
            self.register_update_cache()
        self.msal_app = self.initialize_msal_client()
        self.access_token = self.__authenticate_get_access_token()
        # By this point, the access_token variable should contain the access_token since the auth flow has been completed. Throw exception if still None
        if self.access_token is None:
            raise AzureSphereAPIClient.AzureSphereAPIClientException("Error authenticating with Azure Sphere API. Access token is: " + str(self.access_token))
        self.session = self.init_requests_session()

    # initialize msal client and return the client instance
    def initialize_msal_client(self):
        if os.path.exists(TOKEN_SAVE_FILENAME) and not self.forceinteractive and not self.nocache:
            self.deserialize_cache()
        msal_app = msal.PublicClientApplication(client_id, authority=authority, token_cache=(None if self.nocache else self.cache))
        return msal_app

    # read encrypted cache file, decrypt, then deserialize for use with msal
    def deserialize_cache(self):
        # retrieve the encryption key from the keyring
        with open(TOKEN_SAVE_FILENAME, 'r') as file:
            password:bytes = keyring.get_password(keyring_namespace, keyring_username).encode('utf8')
            fernet = Fernet(password)
            try:
                decrypted_cache = fernet.decrypt(base64.b64decode(file.readline()))
                self.cache.deserialize(decrypted_cache)
            except InvalidToken as e:
                logging.debug("Invalid fernet token. Falling back to interactive auth...")
                logging.debug(e)

    # serialize the msal cache, encrypt, and write to a file
    def write_cache(self):
        password = Fernet.generate_key()
        fernet = Fernet(password)
        encrypted_cache = fernet.encrypt(self.cache.serialize().encode('utf8'))
        keyring.set_password(keyring_namespace, keyring_username, password.decode('utf8'))
        with open(TOKEN_SAVE_FILENAME, 'w') as file:
            file.write(str(base64.b64encode(encrypted_cache), 'utf8'))

    # register atexit handler to update token_cache file before termination
    def register_update_cache(self):
        atexit.register(lambda:self.write_cache() if self.cache.has_state_changed else None)

    # deletes the token_cache file if it exists
    def delete_cache_if_exists(self):
        try:
            os.remove(TOKEN_SAVE_FILENAME)
        except OSError:
            pass

    # create session that retries 3 times on 429 and 500 responses
    def init_requests_session(self):
        retry_strategy = Retry(
            total=3,
            status_forcelist=[500, 429],
        )
        adapter = HTTPAdapter(max_retries=retry_strategy)
        session = requests.Session()
        session.mount("https://", adapter)
        session.mount("http://", adapter)
        return session

    # attempts interactive auth and returns access token if success, else None
    def __interactive_auth(self) -> str:
        self.delete_cache_if_exists()
        print("A local browser window will be opened for you to sign in. CTRL+C to cancel.")
        result = self.msal_app.acquire_token_interactive([scope], prompt="select_account")
        return result

    # authenticates (either using saved access token/refresh token or interactive auth) and return access_token if success, else None
    def __authenticate_get_access_token(self) -> str:
        result = None
        accounts = self.msal_app.get_accounts()
        if accounts:
            account = accounts[0]
            logging.debug("Found saved account: %s" % account["username"])
            # Try to find cached token
            result = self.msal_app.acquire_token_silent(scopes=[scope], account=account)
        if result:
            print("Using saved account: %s" % account["username"])
        else:
            logging.debug("No token found in cache. Falling back to interactive auth...")
            result = self.__interactive_auth()

        if "access_token" in result:
            return result['access_token']
        else:
            logging.error(result.get("error"))
            logging.error(result.get("error_description"))
            return None

    # Sets AllowCrashDumpsCollection for all devicegroup_ids to value of should_set_on. devicegroup_ids are assumed to be in the supplied tenant_id
    def set_allowcrashdumpscollection_devicegroups(self, tenant_id:str, devicegroup_ids :List[str], should_set_on:bool) -> str:
        devicegroups = []
        for devicegroup_id in devicegroup_ids:
            devicegroup = self.patch_devicegroup_allowcrashdumpscollection(tenant_id, devicegroup_id, should_set_on)
            if devicegroup is None:
                logging.error("Error updating device group: " + devicegroup_id)
            else:
                try:
                    devicegroups.append(json.loads(devicegroup))
                except (json.decoder.JSONDecodeError, TypeError) as e:
                    logging.error("Error parsing patch device group " + devicegroup_id + " response")
                    logging.error(e)
                    logging.debug("devicegroup: " + devicegroup)
        return devicegroups

    # print in the case of an error or if verbose logging is enabled
    def log_api_response(self, response):
        if response.status_code != 200:
            try:

                for error in json.loads(response.text):
                    logging.error("Error Name: " + error["ErrorCode"])
                    logging.error("Error Message: " + error["Title"])
            except (KeyError, json.decoder.JSONDecodeError, TypeError):
                logging.error(response.text)
        logging.debug("Received Response Status Code: " + str(response.status_code))
        logging.debug("Response: " + response.text)

    # makes request to Azure Sphere API Device Group - Patch endpoint to update AllowCrashDumpsCollection value to should_set_on. Returns response as json string if true, else None
    # See https://docs.microsoft.com/en-us/rest/api/azure-sphere/devicegroup/patch.
    def patch_devicegroup_allowcrashdumpscollection(self, tenant_id:str, devicegroup_id:str, should_set_on:bool) -> str:
        logging.debug("Patching device group. tenant_id: {0}, devicegroup_id: {1}, should_set_on: {2} ".format(tenant_id, devicegroup_id, should_set_on))
        endpoint="/tenants/"+tenant_id+"/devicegroups/"+devicegroup_id
        url = azure_sphere_api_base_url + endpoint
        data = {"AllowCrashDumpsCollection": should_set_on}
        try:
            response = self.session.patch(url, json=data, headers={"Authorization": "Bearer " + self.access_token})
        except requests.exceptions.RequestException as e:
            logging.error(e)
            return None
        logging.debug(response.status_code)
        self.log_api_response(response)
        return response.text if response.status_code == 200 else None

    # Make call to Azure Sphere API Tenant - List endpoint. Returns response as json string if true, else None
    def list_tenants(self) -> str:
        logging.debug("Listing tenants")
        endpoint = "/tenants"
        url = azure_sphere_api_base_url + endpoint
        try:
            response = self.session.get(url, headers={"Authorization": "Bearer " + self.access_token})
        except requests.exceptions.RequestException as e:
            logging.error(e)
            return None
        self.log_api_response(response)
        return response.text if response.status_code == 200 else None

    # Make call to Azure Sphere API Device Group - List endpoint. Returns response as json string if true, else None
    # See https://docs.microsoft.com/en-us/rest/api/azure-sphere/devicegroup/list
    def list_devicegroups(self, tenant_id:str) -> str:
        logging.debug("Listing device groups. tenant_id: {0}".format(tenant_id))
        endpoint = "/tenants/" + tenant_id + "/devicegroups"
        url = azure_sphere_api_base_url + endpoint
        try:
            response = self.session.get(url, headers={"Authorization": "Bearer " + self.access_token})
        except requests.exceptions.RequestException as e:
            return None
        self.log_api_response(response)
        return response.text if response.status_code == 200 else None

    # Make call to Azure Sphere API Device Group - Get endpoint. Returns response as json string if true, else None
    # See https://docs.microsoft.com/en-us/rest/api/azure-sphere/devicegroup/get
    def get_devicegroup(self, tenant_id:str, devicegroup_id:str) -> str:
        logging.debug("Getting device group. tenant_id: {0}, devicegroup_id: {1}".format(tenant_id, devicegroup_id))
        endpoint = "/tenants/" + tenant_id + "/devicegroups/" + devicegroup_id
        url = azure_sphere_api_base_url + endpoint
        try:
            response = self.session.get(url, headers={"Authorization": "Bearer " + self.access_token})
        except requests.exceptions.RequestException as e:
            logging.error(e)
            return None
        self.log_api_response(response)
        return response.text if response.status_code == 200 else None

def print_arg_error(parser:argparse.ArgumentParser, error_message:str):
    print("\n"+"="*len(error_message)*2)
    print("*ERROR*: "+error_message)
    print("="*len(error_message)*2+"\n")
    if parser is not None:
        parser.print_help()

# validates guid args using regex
def validate_guid(guid_name:str, arg_value:str) -> str:
    if not re.match(r"^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}$", arg_value, flags=re.IGNORECASE):
        print_arg_error(None, "Invalid " + guid_name + ": " + arg_value)
        raise argparse.ArgumentTypeError
    return arg_value

def validate_tenantid(arg_value:str) -> str:
    return validate_guid("tenant id", arg_value)

def validate_devicegroupid(arg_value:str) -> str:
    return validate_guid("device group id", arg_value)

def validate_devicegroupid(arg_value:str) -> str:
    return validate_guid("product id", arg_value)

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Get or set crash dump policy")
    parser.add_argument("--get", action="store_true", default=False, help="Get AllowCrashDumpsCollection")
    parser.add_argument("--set", nargs=1, type=str, choices=["on", "off"], help="Set AllowCrashDumpsCollection")
    parser.add_argument("--tenantid", "-t", type=validate_tenantid, nargs=1, help="Tenant id for which to get or set AllowCrashDumpsCollection")
    parser.add_argument("--devicegroupid", "-dg", type=validate_devicegroupid, nargs="+", help="Device group ids (separated by a space) for which to get or set AllowCrashDumpsCollection")
    parser.add_argument("--verbose", "-v", action="store_true", default=False, help="Enable verbose logging")
    parser.add_argument("--forceinteractive", action="store_true", default=False, help="Force interactive auth instead of trying to load a token from the cache")
    parser.add_argument("--nocache", action="store_true", default=False, help="Don't cache an (encrypted) access token in a file in the current directory")
    args = parser.parse_args()

    if (not args.get and args.set is None) or (args.get and args.set is not None):
        print_arg_error(parser, "Choose either --get or --set")
        return None
    if (args.devicegroupid is not None and args.tenantid is None):
        print_arg_error(parser, "If --devicegroupid is specified, --tenantid must also be specified")
        return None
    return args

# prints Name+Id+AllowCrashDumpsCollection for list of device groups as an ASCII table
def print_devicegroups_table(table:List[str]):
    if not table:
        print("No device groups found.")
        return
    headers = ["Name", "Id", "AllowCrashDumpsCollection"]
    max_column_widths = {}
    #get max width
    for header in headers:
        max_column_widths[header] = max(max(len(str(x[header])) for x in table), len(header))
    #print headers
    print("".join(["{:<{}}\t".format(header, max_column_widths[header]) for header in headers]))
    #print header dividers
    print("".join(["{:<}\t".format("="*max_column_widths[header]) for header in headers]))
    #print rest of table
    for row in table:
        print("".join(["{:<{}}\t".format(str(row[header]), max_column_widths[header]) for header in headers]))

# takes in command line args + authenticated api client and gets or sets AllowCrashDumpsCollection value(s) according to args
def get_or_set_crashdumps_configuration(args:argparse.Namespace, azsphere_api_client):
    should_set_on = args.set is not None and args.set[0] == "on"
    # Confirm first if setting all device groups
    if args.set is not None and args.tenantid is None:
        choice = input("\nThis will " + ("enable" if should_set_on else "disable") + " crash dumps for *all* device groups. Are you sure? [y/n] ")
        if (len(choice) == 0 or choice.lower() not in 'yes'):
            # abort if no confirmation received
            print("Aborting...")
            return
        print(("Enabling" if should_set_on else "Disabling") + " crash dumps for all device groups...")
    try:
        tenants = [(args.tenantid[0], "")] if args.tenantid else [(tenant["Id"], tenant["Name"]) for tenant in json.loads(azsphere_api_client.list_tenants())]
    except (json.decoder.JSONDecodeError, TypeError) as e:
        logging.error("Error retrieving list of tenants.")
        logging.error(e)
        return
    if not tenants:
        print("No tenants found.")
        return
    for tenant_id, tenant_name in tenants:
        print("\nTenant Id: " + tenant_id + ("\nTenant Name: " + tenant_name if tenant_name else ""))
        if args.get:
            # doing --get
            if args.devicegroupid is None:
                # get all device groups
                try:
                    devicegroups = json.loads(azsphere_api_client.list_devicegroups(tenant_id))["Items"]
                    print_devicegroups_table(devicegroups)
                except (json.decoder.JSONDecodeError, TypeError) as e:
                    logging.error("Error getting list of device groups. Make sure the tenant id is correct and the account you selected has access to that tenant id. To switch accounts, run with the --forceinteractive argument.")
                    logging.debug(e)
                    return
            else:
                # get select device groups
                devicegroups = []
                for devicegroupid in args.devicegroupid:
                    try:
                        devicegroups.append(json.loads(azsphere_api_client.get_devicegroup(tenant_id, devicegroupid)))
                    except (json.decoder.JSONDecodeError, TypeError) as e:
                        logging.error("Error getting device group: " + devicegroupid)
                        logging.debug(e)
                        continue
                print_devicegroups_table(devicegroups)
        elif args.set:
            # doing --set
            if args.devicegroupid is None:
                # set all device groups
                try:
                    devicegroupids = [devicegroup["Id"] for devicegroup in json.loads(azsphere_api_client.list_devicegroups(tenant_id))["Items"]]
                except (json.decoder.JSONDecodeError, TypeError) as e:
                    logging.error("Error parsing list of device groups: " + devicegroupid)
                    logging.error(e)
                    return
            else:
                # set select device groups
                devicegroupids = args.devicegroupid
            logging.info("Updating device group(s). This may take a few seconds...")
            devicegroups = azsphere_api_client.set_allowcrashdumpscollection_devicegroups(tenant_id, devicegroupids, should_set_on)
            print_devicegroups_table(devicegroups)
        else:
            logging.error("Unexpected error with get and set arguments")
            return

def configure_logging(is_verbose):
    logging.basicConfig(format='%(levelname)s:%(message)s', level=logging.DEBUG if is_verbose else logging.INFO)
    logging.getLogger("msal").setLevel(logging.INFO if is_verbose else logging.WARN)
    logging.debug("Verbose logging enabled")

def main():
    args = parse_args()
    if args is None:
        return

    if not args.nocache:
        #import fernet and cryptography since they're needed for the encrypted cache
        global Fernet, InvalidToken, keyring
        from cryptography.fernet import Fernet, InvalidToken
        import keyring

    configure_logging(args.verbose)
    logging.debug("args: " + str(args))

    try:
        azsphere_api_client = AzureSphereAPIClient(args.forceinteractive, args.nocache)
    except AzureSphereAPIClient.AzureSphereAPIClientException as e:
        logging.error(e)
        return
    get_or_set_crashdumps_configuration(args, azsphere_api_client)

if __name__ == "__main__":
    main()
