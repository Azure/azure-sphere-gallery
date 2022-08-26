# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

"""This module parses Azure Sphere device binary log file into a human-readable format and the output from running: 'azsphere get-support-data' command will generate device log and other support files"""

import argparse
import os
import sys
import json
import struct
import uuid
import error_code as error_code

class InvalidLogManifestException(Exception):
    pass

class LogParser:
    """ Log storage bank size (in bytes)."""    
    LOG_BANK_SIZE = 4096
    """ Log storage bank counter size (in bytes)."""
    LOG_BANK_COUNTER_SIZE = 2
    """ Log storage bank status size (in bytes)."""
    LOG_BANK_STATUS_SIZE = 2

    def __init__(self, os_version):
        """Initialize the device manifest file path to parse the azure sphere device log binary data

        param: os_version: manifest file directory of released OS version
        """ 
        manifest_dir_path = os.path.dirname(__file__) + "//" + os_version
        if  os.path.isdir(manifest_dir_path):            
            try:                
                self.__manifests = self.__load_manifests(manifest_dir_path)
            except:
                print("manifest file doesn't load properly")
                exit(1)        
        else:
            print("manifest directory path doesn't exists")
            exit(1)

    def __load_manifests(self, path):
        """Iterate manifest file directory and create collection of categoryId manifest message

        param: path: manifest file directory path
        """ 
        manifests = {}
        for dir_name, _, file_names in os.walk(path):
            for file_name in file_names:
                if file_name.endswith(".json"):
                        manifest = self.__load_manifest(os.path.join(dir_name, file_name))                    
                if manifest:
                    if manifest["categoryId"] in manifests:
                        raise InvalidLogManifestException()

                    manifests[manifest["categoryId"]] = manifest
        return manifests

    def __load_manifest(self, path):
        """Iterate manifest file content of each file and assign the  relevant data. 

        param: path: manifest file directory path
        """ 
        with open(path) as contents:
            data = json.load(contents)
            data["messages"] = {}

            # Iterate the manifest content, If manifest msgIds available, load the message details  
            if "msgIds" in data:
                for message in data["msgIds"]:
                    if "msgId" in message:
                        message_id = message["msgId"]
                        data["messages"][message_id] = message

            return data

    def __try_get_category(self, category_id):
        """Get the manifest message for a category id. 

        param: category_id: category_id of the manifest message
        """ 
        if category_id not in self.__manifests:
            return None

        return self.__manifests[category_id]

    def try_get_category_name(self, category_id):
        """Get the manifest message for a category id and return category name. 

        param: category_id: category_id of the manifest message
        """ 
        category = self.__try_get_category(category_id)
        return category["categoryName"] if (category and "categoryName" in category) else None

    def try_format(self, category_id, message_id, parameters):
        """This function is used to format the manifest message. 

        param: category_id: category_id of the manifest message
        message_id: message_id of the manifest message
        parameters: if parameters present on the message expand the message
        """ 
        category = self.__try_get_category(category_id)
        if not category:
            return None

        if message_id not in category["messages"]:
            return None

        message = category["messages"][message_id]
        if not message:
            return None

        message_str = message["msgString"] if "msgString" in message else None
        if not message_str:
            return None

        message_params = message["params"] if "params" in message else None

        def expand_param(index, parameter):
            if message_params and index < len(message_params):
                message_param = message_params[index]
                if message_param and "paramName" in message_param:
                    message_param_name = message_param["paramName"]
                    if message_param_name:
                        return "{}: {}".format(message_param_name, parameter)

            return parameter

        result = message_str

        if parameters:
            result += " ("

            i = 0
            for parameter in parameters:
                if i > 0:
                    result += ", "

                result += expand_param(i, parameter)
                i += 1

            result += ")"

        return result

    def __get_priority_string(self, priority):
        """Convert priority number to priority string"""
        return {
            0: 'None',
            1: 'Fatal',
            2: 'Error',
            3: 'Warning',
            4: 'Info',
            5: 'Debug',
            6: 'Trace'
        }[priority]

    def __get_param_string(self, param, data, index):
        """Get string from param"""
        result_string = ("%s: " % param['paramName'])
        if param['paramType'] == 'integer':
            (value) = struct.unpack('@i', data[index: index + 4])
            result_string += ("%d" % value)
            index += 4
        if param['paramType'] == 'ipAddress':
            (value1, value2, value3, value4) = struct.unpack('@BBBB', data[index:index + 4])
            result_string += ("%d.%d.%d.%d" % (value1, value2, value3, value4))
            index += 4
        if param['paramType'] == 'uniqueId':
            value = data[index:index + 16]
            guid = uuid.UUID(bytes=value)
            result_string += ("%s" % guid)
            index += 16
        if param['paramType'] == 'unsignedInteger':
            (value) = struct.unpack('@I', data[index: index + 4])
            result_string += ("%u" % value)            
            index += 4
        if param['paramType'] == 'commonError':
            (value) = struct.unpack('@I', data[index: index + 4])
            errname = error_code.parse_error_code(("%u" % value))           
            result_string += repr(errname) 
            index += 4
        if param['paramType'] == 'string':
            end = data.find(b'\0', index)
            value = data[index:end].decode()
            result_string += ("%s" % value)
            index = end + 1
        return (result_string, index)

    def __find_first_log_entry(self, file_content):
        """Find the first log message, knowing that the circular buffer wraps.

        param: file_content: log message content
        """ 
        offset = -1
        smallest_bank_counter = 0xFFFF
        index = 0
        while index < len(file_content):
            (bank_counter, bank_status) = struct.unpack('@HH', file_content[index + self.LOG_BANK_SIZE - self.LOG_BANK_COUNTER_SIZE - self.LOG_BANK_STATUS_SIZE : index + self.LOG_BANK_SIZE])

            """ Look for the smallest bank counter.  Ignoring roll-over, this is the start of the log. """
            if bank_counter < smallest_bank_counter:
                smallest_bank_counter = bank_counter
                offset = index

            index += self.LOG_BANK_SIZE

        return offset

    def __parse_log_entries(self, file_content, start_offset, stop_offset):
        """Parse log data of the  message

         param: file_content: log message content
        """ 
        manifests = self.__manifests
        if not manifests:
            return None

        result = []

        """ Index past the block attributes. """
        index = start_offset
        bank = start_offset // self.LOG_BANK_SIZE
        while index < stop_offset:

            """ Determine whether there's a valid message to decode.  There needs to be at least enough space to store """
            """ the 12-byte message header and for the trailing bank footer (counter & status) to fit.                 """
            if ((index % self.LOG_BANK_SIZE) >= (self.LOG_BANK_SIZE - 12 - self.LOG_BANK_COUNTER_SIZE - self.LOG_BANK_STATUS_SIZE)):
                bank += 1
                index = (bank * self.LOG_BANK_SIZE)
                continue

            (msg_length, _, priority, category_id, msg_id, time_stamp) \
                = struct.unpack('@HBBHHI', file_content[index:index + 12])

            param_string = ""

            """ Determine whether there's a valid message to decode.  A message length of 0xFFFF indicates that """
            """ there are more more messages in this bank.                                                      """
            if msg_length == 0xFFFF:
                bank += 1
                index = (bank * self.LOG_BANK_SIZE)
                continue

            manifest = {'categoryName':f'Unknown category id {category_id}'}
            msg = {'msgString':f'Unknown message ID {msg_id}'}

            match = [item for item in manifests.values() if item['categoryId'] == category_id]
            if match != []:
                manifest = match[0]

                match = [item for item in manifest['msgIds'] if item['msgId'] == msg_id]
                if match==[]:
                    if msg_length > 12:
                        for i in range(index+12,index+msg_length):
                            param_string += hex(file_content[i]) + ' '
                        param_string = '(Params: ' + param_string + ')'
                else:
                    msg = match[0]

            if 'params' in msg:
                param_string = ""
                param_index = index + 12
                for param in msg['params']:
                    if param_string:
                        param_string += ", "

                    if param_index - index > msg_length:
                        sys.stderr.write("ERROR: Invalid parameter for message id: %d,"\
                            " category id: %d" % (msg_id, category_id))
                        return None

                    (current_param_string, param_index) = self.__get_param_string(param, file_content, param_index)
                    param_string += current_param_string

                param_string = "(" + param_string + ")"

            time_stamp_str = ("%.6f" % (time_stamp / 1000)).rjust(12)

            result.append("[%s] %s %s(%s): %s %s" % (
                time_stamp_str,
                self.__get_priority_string(priority),
                manifest['categoryName'],
                msg_id,
                msg['msgString'],
                param_string))
            index += msg_length

        return result

    def parse_log(self, log_contents):
        """Parse log data of the  message

         param: file_content: log message content
        """ 
        offset = self.__find_first_log_entry(log_contents)

        if offset == -1:
            sys.stderr.write("WARNING: Unable to find first log message.\n")
            return None

        result = self.__parse_log_entries(log_contents, offset, len(log_contents))
        if result is None:
            return None

        if offset != 0:
            next_result = self.__parse_log_entries(log_contents, 0, offset)
            if next_result is None:
                return None

            result += next_result

        return result

    def dump_log(self, log_contents) -> bool:
        """Load the parsed device log content and print the  message

         param: log_contents: device log message content
        """ 
        result = self.parse_log(log_contents)
        if not result:
            return False

        for line in result:
            print(line)

        return True


os_version = ''
def parse_arguments():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser(description='Parse binary device log into human-readable format')

    parser.add_argument('-d', '--version',
                        help='OS version e.g: 22.07',
                        type=str, dest='os_version', required='true')  

    parser.add_argument('-f', '--binaryfilename',
                        help='Azure sphere device log binary filename e.g: AzureSphere_DeviceLog_113.bin',
                        type=str, dest='bin_filename', required='true') 

    args = parser.parse_args()

    return args

if __name__=='__main__':
    """Entry point of the program to parse device log content and print the  message."""
         
    args = parse_arguments()
    file_content = ""
   
    try:
        with open(args.bin_filename, mode='rb') as file: # b is important -> binary    
            file_content = file.read() 
    except:
        print('device log file couldnt be opened to read properly')

    parser = LogParser(args.os_version)
    if parser:
        if not parser.dump_log(file_content):
            exit(1)
    else:
        print("manifest list is empty")

    exit(0)
