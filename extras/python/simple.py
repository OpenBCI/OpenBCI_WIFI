#!/usr/bin/env python

import socket
import json

TCP_IP = '192.168.0.14'
TCP_PORT = 5005
BUFFER_SIZE = 3000  # Normally 1024, but we want fast response

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind((TCP_IP, TCP_PORT))
s.listen(1)

conn, addr = s.accept()
print 'Connection address:', addr
while 1:
    data = conn.recv(BUFFER_SIZE)
    try:
        json_object = json.loads(data)
        chunk = json_object["chunk"]
        for sample in chunk:
            print sample["sampleNumber"]
    except BaseException as e:
        pass
        # print e
    if not data: break
    # print "received data:", data
    # conn.send(data)  # echo
conn.close()
