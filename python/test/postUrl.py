#!/usr/bin/env python
# -*- coding: utf-8

import urllib 
import urllib2 
import cookielib
from bs4 import BeautifulSoup

'''
url = 'http://www.pythontab.com' 
user_agent = 'Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)' 
  
values = {'name' : 'Michael Foord', 
          'location' : 'pythontab', 
          'language' : 'Python' } 
headers = { 'User-Agent' : user_agent } 
data = urllib.urlencode(values) 
req = urllib2.Request(url, data, headers) 
response = urllib2.urlopen(req) 
the_page = response.read()
print the_page

'''
if __name__ == "__main__":
    url = 'http://www.renren.com/SysHome.do'
    resp1=urllib2.urlopen(url)
    source=resp1.read()
    soup1=BeautifulSoup(source,"html.parser")
    log_url=soup1('form', {'method':'post'})[0]['action']
    info={}
    cj=cookielib.CookieJar()
    opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(cj))
    urllib2.install_opener(opener)
    
    try:
        resp2=urllib2.urlopen(log_url,urllib.urlencode(info))
    except urllib2.URLError,e:
        if hasattr(e,'reson'):
            print 'reson:{0}'.format(e.reson)
        if hasattr(e,'code'):
            print 'code:{0}'.format(e.code)


