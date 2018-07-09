#!/usr/bin/env python
# -*- coding: utf-8
#select 

import socket
import Queue
from select import select

if __name__ == "__main__":
    client = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    client.connect(("127.0.0.1",9999))
    
    while 1:
        cmd = raw_input("==>:")
        #print cmd
        if len(cmd) == 0:
            continue
            
        client.send(cmd.encode("utf-8"))
        cmd_rsv = client.recv(1024)
        print cmd_rsv
        
    client.close


