# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import requests
from azuresphere_device_api import devices, image


def get_device_os_version():
    """The main entry point of the program that finds the OS version."""
    # Get the list of attached devices IP addresses
    attached_devices = devices.get_attached_devices()
    
    # Get the list of Azure Sphere published OS images
    versions = _get_published_os_list()
    
    for device in attached_devices:
        device_ip = device["IpAddress"]
        os_version = _get_os_version(versions, _get_device_images(_get_device_image_list(device_ip)))
        print(f"Device IP = {device_ip}, OS version = {os_version}")
    
def _get_published_os_list() -> list:
    """Get the Azure Sphere OS Versions and their Image and Component IDs.

    :returns: An array of OS versions and their Image and Component IDs.
    :rtype: list
    """
    response = _get_request("https://prod.releases.sphere.azure.net/versions/mt3620an.json")
    return response['versions']

def _get_device_image_list(device_ip:str)-> list:
    """Gets a list of components installed on the device.

    :param device_ip: The IP address of the device you would like to retrieve the image list from.
    :type device_ip: str
    :returns: The list of components running on the device.
    :rtype: list
    """
    devices.set_active_device_ip_address(device_ip)
    response = image.get_images()
    
    return response['components']
    
def _get_device_images(components: list) -> dict:
    """Get a list of images on the device that are not applications.

    :param components: The array of components you would like to retrieve the device images from.
    :type components: list
    
    :returns: The images with any applications filtered out.
    :rtype: dict
    """
    images = {}
    for component in components:
        # Image types https://docs.microsoft.com/rest/api/azure-sphere/image/get-metadata
        if component['image_type'] != 10:
            images[component['name']] = component['uid']
    
    return images

def _get_os_version(version:list, device_images:dict):
    """Gets the current OS Version for the device by matching device image IDs to the published OS Image IDs.

    :param version: The OS versions you are comparing the device against.
    :type version: list
    :param device_images: The device images running on your device.
    :type device_images: dict
    
    :returns: The OS version your device is running if found, otherwise an empty string.
    :rtype: str
    """
    device_images_key_list = list(device_images.keys())
    for x in reversed(range(len(version))):
        for y in range(len(version[x]['images'])):
            for z in range(len(device_images)): 
                if version[x]['images'][y]['cid'] == device_images[device_images_key_list[z]]:
                    return version[x]['name']
    
    return ""

def _get_request(address: str) -> str:
    """Makes a "GET" request to the given address.

    :param address: The address to make the "GET" request to.
    :type address: str
    
    :returns: The content of the response, as a string.
    :rtype: str
    """
    response = requests.get(address)
    return response.json()

get_device_os_version()
