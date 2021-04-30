# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the MIT License.

import os
import time
import paho.mqtt.client as paho
import ssl
from pathlib import Path

broker_address='test.mosquitto.org'

def on_message(client, userdata, message):
  msg=str(message.payload.decode("utf-8"))
  print("received message =",msg)
  msg=msg[len(msg)-1]+msg[0:len(msg)-1]
  client.publish("azuresphere/sample/device",msg)

def on_connect(client, userdata, flags, rc):
  print("Connected ")
  client.subscribe("azuresphere/sample/host")

client=paho.Client() 
client.on_message=on_message
client.on_connect=on_connect
print("connecting to broker")

parentDir=os.path.abspath(os.path.join(Path(os.path.dirname(os.path.realpath(__file__))), os.pardir))
certsDir=os.path.join(parentDir,'HighLevelApp','Certs')
print(certsDir)
if (os.path.exists(os.path.join(certsDir,'mosquitto.org.crt'))):
    print('mosquitto cert exists')
else:
    print("mosquitto cert doesn't exist :(")

client.tls_set(ca_certs=os.path.join(certsDir,'mosquitto.org.crt'), certfile=os.path.join(certsDir,'client.crt'), keyfile=os.path.join(certsDir,'client.key'), cert_reqs=ssl.CERT_REQUIRED, tls_version=ssl.PROTOCOL_TLSv1_2)
client.tls_insecure_set(False)
client.connect(broker_address, 8883, 60)

##start loop to process received messages
client.loop_forever()



