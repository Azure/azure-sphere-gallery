# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

#!/usr/bin/env python
# encoding: utf-8
import json
import os
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

print("Littlefs disk host")

# create empty 'disk'
# could easily modify this to read from a file
diskData=bytearray(4194304)
print("data length",len(diskData))

print('startup memory CRC')
memoryCRC()

# -------------------------------------------------------------------------------------
# Block level read and write.

@app.route('/ReadBlockFromOffset', methods=['GET'])
def query_sector():
    offset = request.args.get('offset')
    size= request.args.get('size')

    if not offset or not size:
        response=make_response(jsonify({'error': 'read request is not valid'}),400)
        return response
    else:
        diskOffset=int(offset)
        blockSize=int(size)

        print("Request for offset: ",hex(diskOffset))
        returnData=diskData[diskOffset:diskOffset+blockSize]

        crc=0
        for val in returnData:
            crc=crc+val
            crc=crc & 0xffff

        print("Read CRC", hex(crc))
        hexDump(returnData,diskOffset)

        response = make_response(returnData,200)
        response.headers.set('Content-Type', 'application/octet-stream')
        return response

@app.route('/WriteBlockFromOffset', methods=['POST'])
def write_sector():
    print("Content Length: ",request.content_length)
    print("Content Type  : ", request.content_type)
    print("Headers       : ", request.headers)

    offset=request.headers['offset']

    if not offset:
        response=make_response(jsonify({'error': 'write request is not valid'}),400)
        return response
    else:
        blockOffset=int(offset)
        chunk = request.stream.read(request.content_length)    # read a sector

        print("chunk type: ",type(chunk))
        print("chunk len : ",len(chunk))

        print("save ",len(chunk),"bytes to ",hex(blockOffset))

        crc=0
        for val in chunk:
            crc=crc+val
            crc=crc & 0xffff

        print("CRC", hex(crc))

        print('Update disk image')
        # update the disk image
        diskData[blockOffset: blockOffset+len(chunk)]=chunk

        print('---------------------------------------------------------')

        memoryCRC()
        hexDump(chunk,blockOffset)

        print('validate new CRC')
        validateCRC=0
        for x in range(request.content_length):
            validateCRC=validateCRC+diskData[blockOffset+x]
            validateCRC=validateCRC & 0xffff

        print("Validate CRC", hex(validateCRC))

        response=make_response("OK",200)
        return response

@app.route('/WriteDisk', methods=['GET'])
def write_disk():
    fp=open("TestDisk.dsk","wb")
    fp.write(diskData)
    fp.close()

    response=make_response(jsonify({'write': 'ok'}),200)
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