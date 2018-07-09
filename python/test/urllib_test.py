# -*- coding:utf-8 -*-  
#!/usr/bin/python
'''
import urllib 
import urllib2 
url = 'http://www.pythontab.com' 
user_agent = 'Mozilla/4.0 (compatible; MSIE 5.5; Windows NT)' 
values = {'name' : 'Michael Foord', 
          'location' : 'pythontab', 
          'language' : 'Python' } 
headers = { 'User-Agent' : user_agent } 		  
data = urllib.urlencode(values) 
req = urllib2.Request(url, data) 
try:
	response = urllib2.urlopen(req) 
	the_page = response.read()
except URLError,e:
	print e.reason
'''


import urllib  
import urllib2  
import cookielib  


hosturl = 'http://www.renren.com/'  

#post数据接收和处理的页面，点登陆的时候http POST的链接
posturl = 'http://www.renren.com/ajaxLogin/login?1=1&uniqueTimestamp=20151021027896'  

#设置一个cookie处理器，它负责从服务器下载cookie到本地，并且在发送请求时带上本地的cookie  
cj = cookielib.LWPCookieJar()  
cookie_support = urllib2.HTTPCookieProcessor(cj)  
opener = urllib2.build_opener(cookie_support, urllib2.HTTPHandler)  
urllib2.install_opener(opener)  

#打开登录主页面（他的目的是从页面下载cookie，这样我们在再送post数据时就有cookie了，否则发送不成功）  
h = urllib2.urlopen(hosturl)  

#构造header，一般header至少要包含一下两项。这两项是从抓到的包里分析得出的。  
headers = {  
'User-Agent':'Mozilla/5.0 (Windows NT 6.1; WOW64; rv:42.0) Gecko/20100101 Firefox/42.0',  
'Referer' : 'http://www.renren.com/'  
}  

#抓包获取参数,password一般会通过某种方式加密。  
postData = {  
'email':'xxxxxx',  
'icode':'',  
'origURL':'http://www.renren.com/home',  
'domain':'renren.com',  
'key_id':1,  
'captcha_type':'web_login',  
'password':'xxxxxx',  
'rkey':'70153f87e588d34f588cccecdd878d66',  
'f':''  
}

postData = urllib.urlencode(postData)  

#通过urllib2提供的request方法来向指定Url发送我们构造的数据，并完成登录过程    
request = urllib2.Request(posturl, postData, headers)  

print request  
response = urllib2.urlopen(request)  
text = response.read()  
text = text.replace("true","1").replace("false","0")  
text = eval(text)  
url = text.get('homeUrl')  # 这个才是你人人主页的真实url  
if not url:  
	print "error"  

response = urllib2.urlopen(url)  

result = response.read().decode("utf-8").encode("gbk")  
#print result

