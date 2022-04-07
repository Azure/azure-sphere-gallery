import logging
import requests
import json
from azure.identity import DefaultAzureCredential
from requests.structures import CaseInsensitiveDict

import azure.functions as func
AzureSphereApiUri = "https://prod.core.sphere.azure.net"

def GetAS3SData(relativeUrl):
    logging.info('Getting AS3 data')
    scope="https://firstparty.sphere.azure.net/api/.default" 
    default_credential = DefaultAzureCredential(managed_identity_client_id="YOUR_FUNCTION_APP_SYSTEM_ASSIGNED_MANAGED_IDENTITY")
    logging.info('Have default_credential')
    token=default_credential.get_token(scope)
    if (not token):
        logging.info("Didn't get a token")
        return b'[]'

    AS3Token=token.token
    logging.info('Have a token...')
    logging.info(AS3Token)

    url=AzureSphereApiUri+f'/v2/{relativeUrl}'
    headers = CaseInsensitiveDict()
    headers["Accept"] = "application/json"
    headers["Authorization"] = f"Bearer {AS3Token}"

    logging.info('calling requests.get')

    resp=b''
    try:
        resp = requests.get(url, headers=headers)
    except: 
        logging.info('AS3 call failed :(')
        return b'[]'

    return resp.content

def main(req: func.HttpRequest) -> func.HttpResponse:
    logging.info('Python HTTP trigger function processed a request.')
    tenants=GetAS3SData('Tenants')
    logging.info('back from AS3 call')
    jstring = tenants.decode('ascii') # Decode using ascii encoding
    logging.info(jstring)
    d = json.loads(jstring)

    logging.info('output array to user')

    resp='Function triggered ok\r\n'
    for user_tenants in d:
        resp+=user_tenants["Id"]+' '+user_tenants["Name"]+'\r\n'
        logging.info(user_tenants["Id"])
        logging.info(user_tenants["Name"])

    return func.HttpResponse(
         resp,
         status_code=200
    )