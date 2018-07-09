#!/usr/bin/env python
# -*- coding: utf-8
import string
import json
import sys
import os
import math
import random

from ctypes import *
import re

class SLineInfo:
    pass;

pfile = open("E:\\code\\python\\length\\text.txt", "ab+")
linecurr = "0"

def resovle(info):
    try:
        indexstart = info.index(',')
    except ValueError:
        return 0
    subHead = info[indexstart+1:]
    try:
        indexend = subHead.index(',')
    except ValueError:
        return 0
    linenum = subHead[0:indexend]
    try:
        indexinfo = info.index(']')
    except ValueError:
        return 0

    strid = subHead[indexstart+1:]
    ts = SLineInfo()
    ts.start = indexinfo + 1
    ts.id = linenum
    return ts

def pasline( info):
    ms = resovle(info)
    if 0 == ms:
        return
    if linecurr != ms.id:
        pfile.write("\r\n")
        global linecurr
        linecurr = ms.id
    substr = info[ms.start:]
    pfile.write(substr)
    pass

def pasfile(filename):
    file = open(filename, "rb")
    for line in file:
        if len(line ) > 0:
            line =line.strip('\n')
            line =line.strip('\r')
            pasline(line)
    pass



if __name__ == "__main__":
    #str = "[112341242134dasdas,12121,3444,5566,jsjadja]"
    pasfile("E:\\code\\python\\length\\100000000.txt")
    pass