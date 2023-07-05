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
BLOCK_COUNT = DISK_SIZE // BLOCK_SIZE
assert(DISK_SIZE % BLOCK_SIZE == 0)

print("Disk: {:d} x {:d} bytes = {:d} bytes".format(BLOCK_COUNT, BLOCK_SIZE, DISK_SIZE))

class Block:
    def __init__(self):
        self.data = bytes(BLOCK_SIZE)
        self.metadata = bytes(METADATA_SIZE)

# create empty 'disk'
# could easily modify this to read from a file
diskData = [Block() for _ in range(0, BLOCK_COUNT) ]

# -------------------------------------------------------------------------------------
# Block level read and write.

@app.route('/ReadBlock', methods=['GET'])
def query_sector():
    blockNum = request.args.get('block')

    if not blockNum:
        response=make_response(jsonify({'error': 'Missing block arg in block read'}),400)
        return response
    else:
        iBlockNum = int(blockNum)
        data     = diskData[iBlockNum].data
        metaData = diskData[iBlockNum].metadata

        print("Read block {:d}".format(iBlockNum))
        hexDump(data, 0)
        print("Metadata:")
        hexDump(metaData, 0)

        responseData = data + metaData
        response = make_response(responseData,200)
        response.headers.set('Content-Type', 'application/octet-stream')
        return response

@app.route('/WriteBlock', methods=['POST'])
def write_sector():
    blockNum = request.args.get('block')
    
    if not blockNum:
        response=make_response(jsonify({'error': 'Missing block arg in block write'}),400)
        return response

    requestdata = request.stream.read()
    if len(requestdata) != BLOCK_SIZE + METADATA_SIZE:
        response=make_response(jsonify({'error': 'Incorrect data size {:d} bytes for block write'.format(len(requestdata))}),400)
        return response

    data = requestdata[:BLOCK_SIZE]
    metadata = requestdata[BLOCK_SIZE:]

    iBlockNum = int(blockNum) 
    diskData[iBlockNum].metadata = metadata
    diskData[iBlockNum].data = data

    print("Write block {:d}:".format(iBlockNum))
    hexDump(diskData[iBlockNum].data, 0)
    print("Metadata:")
    hexDump(diskData[iBlockNum].metadata, 0)

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