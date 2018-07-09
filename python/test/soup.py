#!/usr/bin/env python
# -*- coding: utf-8
import urllib 
import urllib2 
import cookielib
from bs4 import BeautifulSoup


soup = BeautifulSoup(open('12.html'),"html.parser")

ms = soup.head
print '1--------------------------------------------------->'
print soup.head.contents[1]
print soup.head.contents[2]
print soup.head.contents[3]
print soup.head.contents[4]
print soup.head.contents[5]
print soup.head.contents[6]
print soup.head.contents[7]
print soup.head.contents[8]
print soup.head.contents[9]
print soup.head.contents[10]
print soup.head.contents[11]
print soup.head.contents[12]
print soup.head.contents[13]
print soup.head.contents[14]
print soup.head.contents[15]
print soup.head.contents[16]
print soup.head.contents[17]
print soup.head.contents[18]
print soup.head.contents[19]
print '2--------------------------------------------------->'
print ms.nextSibling
print '3--------------------------------------------------->'
print ms.nextSibling.nextSibling
print '4--------------------------------------------------->'

'''
for bodyChild in soup.body.children:
	ms = bodyChild.nextSibling
	if ms is None:
		print '===>'
	else:
		print ms.string
#ns = bodyattrs.keys()
#for key in ns:
#	print key
'''