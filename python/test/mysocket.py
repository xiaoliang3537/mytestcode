#!/usr/bin/env python
# -*- coding: utf-8

import socket
import time
import struct
import base64
import sys
from select import select
import re
import logging
from threading import Thread
import signal


s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(('www.sina.com.cn', 80))
s.send('GET / HTTP/1.1\r\nHost: www.sina.com.cn\r\nConnection:close\r\n\r\n')
buffer = []
while True:
    d = s.recv(1024)
    if d:
        buffer.append(d)
    else:
        break

s.close()
data = ''.join(buffer)

if __name__ == "__main__":
    print(data)

