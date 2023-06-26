# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#!/usr/bin/env python
# encoding: utf-8
import json
import os
import base64
from pathlib import Path
from flask import Flask, request, jsonify, make_response, session
import datetime

def memoryCRC():
    memCRC=0
    for val in diskData:
        memCRC=memCRC+val
        memCRC=memCRC & 0xffff
    print('memCRC : ',hex(memCRC))

app = Flask(__name__)

print("Python disk host")

DISK_SIZE = 4 * 1024 * 1024
BLOCK_SIZE = 256 
METADATA_SIZE = 16
assert(DISK_SIZE % BLOCK_SIZE == 0)
BLOCK_COUNT = DISK_SIZE // BLOCK_SIZE

print("Disk: {:d} x {:d} bytes = {:d} bytes".format(BLOCK_COUNT, BLOCK_SIZE, DISK_SIZE))

class Block:
    def __init__(self):
        self.bytes = bytes(BLOCK_SIZE)
        self.metadata = bytes(METADATA_SIZE)

Disk = list[Block]

# create empty 'disk'
# could easily modify this to read from a file
diskData: Disk = [Block()] * BLOCK_COUNT

# -------------------------------------------------------------------------------------
# Block level read and write.

@app.route('/ReadBlock', methods=['GET'])
def query_sector():
    blockNum = request.args.get('block')

    if not blockNum:
        response=make_response(jsonify({'error': 'read request is not valid'}),400)
        return response
    else:
        iBlockNum = int(blockNum)
        returnData=diskData[iBlockNum].bytes
        metaData = diskData[iBlockNum].metadata
        returnMetaData = base64.b64encode(metaData)

        print("Read block {:d}".format(iBlockNum))
        hexDump(returnData, 0)
        print("Metadata:")
        hexDump(metaData, 0)

        response = make_response(returnData,200)
        response.headers.set('Content-Type', 'application/octet-stream')
        response.headers.set('Block-Metadata', returnMetaData)
        return response

@app.route('/WriteBlock', methods=['POST'])
def write_sector():
    print("Content Length: ",request.content_length)
    print("Content Type  : ", request.content_type)
    print("Headers       : ", request.headers)
    
    headers = request.headers.keys()
    if not "Block-Num" in headers or not "Block-Metadata" in headers:
        response=make_response(jsonify({'error': 'Missing block number or metadata'}),400)
        return response
    
    data = request.stream.read()
    if len(data) != BLOCK_SIZE:
        response=make_response(jsonify({'error': 'Incorrect data size for block write'}),400)
        return response

    blockNum=int(request.headers['Block-Num'])
    metadata=base64.decodebytes(request.headers['Block-Metadata'])

    diskData[blockNum].metadata = metadata
    diskData[blockNum].bytes = data

    print("Write block {%d}:".format(blockNum))
    hexDump(data, 0)
    print("Metadata:")
    hexDump(metadata, 0)

    response=make_response("OK",200)
    return response
    
def hexDump(data, offset, banner=False):
    linebreak=0
    lineCounter=0
    if banner:
        print('---------------------------------------------')
        print(datetime.datetime.now().strftime('%H:%M:%S')+' - ', end='')
        print('Size:', str(len(data)))
    asciiData=''
    for byte in data:
        if (linebreak == 0):
            asciiData=''
            print('{:04x}'.format(lineCounter+offset)+': ',end='')
            lineCounter+=16
        print('{:02x}'.format(byte)+' ',end='')
        if (byte < 0x20 or byte > 0x7f):
            asciiData+=str('.')
        else:
            asciiData+=str(chr(byte))

        linebreak+=1
        if (linebreak == 16):
            linebreak=0
            print('  '+asciiData)

    if linebreak > 0:
        print(' '*((16-linebreak)*3), end='')
        print('  '+asciiData)

app.run(host='0.0.0.0')