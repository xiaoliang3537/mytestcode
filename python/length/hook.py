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

dt = dict()

class SInfo:
    pass;

# 解析字符串
def resovle(info):
    try:
        indexend = info.index(']')
    except ValueError:
        return 0
    subHead = info[1:indexend]
    indexstart = subHead.rfind(',')
    strid = subHead[indexstart+1:]
    ts = SInfo()
    ts.start = indexend+1
    ts.id = strid
    return ts

def pasline( info):
    ms = resovle(info)
    if 0 == ms:
        return
    try:
        key = dt.keys().index(ms.id)
    except ValueError:
        strFileName = str(ms.id) +".txt"
        file = open(strFileName, "wb")
        dt[ms.id] = file
    value = dt[ms.id]
    #value.write(info[ms.start:])
    value.write(info)
    pass

def pasfile(filename):
    file = open(filename, "rb")
    for line in file:
        if len(line ) > 0:
            pasline(line)

# 关闭文本

if __name__ == "__main__":
    #str = "[112341242134dasdas,12121,3444,5566,jsjadja]"
    pasfile("E:\\code\\python\\length\\textout.log")
    pass
