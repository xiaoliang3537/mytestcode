#!/usr/bin/env python
# -*- coding: utf-8

import urllib  
import urllib2  
import cookielib
import chardet
from bs4 import BeautifulSoup

'''
url = 'http://mail.163.com'
user_agent = 'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 6.1; Trident/5.0; SLCC2;.NET CLR 2.0.50727; .NET CLR 3.5.30729; .NET CLR 3.0.30729; Media Center PC 6.0;InfoPath.3; KB974488) '  
values = {'username' : 'xiaoliang3537@163.com',  'password' : 'bkbmkb39006500' }  
headers = { 'User-Agent' : user_agent }  
data = urllib.urlencode(values)  
request = urllib2.Request(url, data, headers)  
response = urllib2.urlopen(request)  
page = response.read() 

filename = 'cookie.txt'
cookie = cookielib.MozillaCookieJar(filename)
cookie.save(ignore_discard=True, ignore_expires=True)

print page
'''

values={}
values['username'] = "xiaoliang3537@163.com"
values['password']="bkbmkb39006500"
data = urllib.urlencode(values) 
url = "http://dl.reg.163.com"
geturl = url + "?"+data
request = urllib2.Request(geturl)
response = urllib2.urlopen(request)
s = response.read()

print s

