#!/usr/bin/env python
# -*- coding: utf-8

from ctypes import *   
import re


if __name__ == "__main__":
    dll = CDLL("axmlmodify.dll")  
    ret = dll.parseAxml("E:\code\python\AndroidManifest.xml", "E:\code\python\AndroidManifest1.xml")  
    print ret
'''
class test:
	def __init__(self, name,code):
		self.name = name
		self.code = code
	def _getCode(self):
		return self.code
	def _getName(self):
		return self.name

	def _readFile(self, path):
		with open('E:\\code\\python\\cqc.py', 'rb') as f:
			for line in f:	
				str = '%s' %line.decode("utf-8")
				print str.strip('\n')
		
s = test(1,2)
s._readFile(1)
'''

	

