#!/usr/bin/python
# -*- coding:utf-8 -*-

import urllib
import urllib2
import cookielib
#import request
import sys
import os
import pdb
import re
import json
import getopt
#pdb.set_trace()

class CookieMgr:
    def __init__(self):
        self.cookie = None
        self.filename = 'cookie.txt'
    def getCookie(self):
        if(self.cookie == None):
            self.loadCookie()
        if(self.cookie == None):
            self.requestCookie()
        return self.cookie
    def loadCookie(self):
        self.cookie = cookielib.MozillaCookieJar()
        self.cookie.load(self.filename, ignore_discard=True, ignore_expires=True)
        for item in self.cookie:
            print 'LoadCookie: Name = '+item.name
            print 'LoadCookie:Value = '+item.value
    def requestCookie(self):
        #声明一个MozillaCookieJar对象实例来保存cookie，之后写入文件
        self.cookie = cookielib.MozillaCookieJar(self.filename)
        opener = urllib2.build_opener(urllib2.HTTPCookieProcessor(self.cookie))
        postdata = urllib.urlencode({
                    'LoginForm[email]':'xxxxxxx',
                    'LoginForm[password]':'xxxxxx',
                    'LoginForm[rememberMe]':'0',
                    'LoginForm[rememberMe]':'1',
                    'yt0':'登录'
                })
        loginUrl = 'http://www.myhuiban.com/login'
        #模拟登录，并把cookie保存到变量
        result = opener.open(loginUrl,postdata)
        #保存cookie到cookie.txt中
        self.cookie.save(ignore_discard=True, ignore_expires=True)
        for item in self.cookie:
            print 'ReqCookie: Name = '+item.name
            print 'ReqCookie:Value = '+item.value
    def badCode(self):
        #server return 500 internal error, not use, just show tips
        login_url = 'http://www.myhuiban.com/conferences'
        #利用cookie请求访问另一个网址，此网址是成绩查询网址
        #gradeUrl = 'http://www.myhuiban.com/conferences'
        #请求访问成绩查询网址
        #result = opener.open(gradeUrl)
        #print result.read()
    def test(self):
        mgr = CookieMgr()
        cookie = mgr.getCookie()
    def directCookie(self):
        #在http://www.myhuiban.com/login登录页面，发起网络请求，通过浏览器自带的开发者工具，观测网络发送的数据包，获取cookie
        cookies = {
            'PHPSESSID':'3ac588c9f271065eb3f1066bfb74f4e9',
            'cb813690bb90b0461edd205fc53b6b1c':'9b40de79863acaa3df499703611cdb1e123b15c9a%3A4%3A%7Bi%3A0%3Bs%3A14%3A%22lpstudy%40qq.com%22%3Bi%3A1%3Bs%3A14%3A%22lpstudy%40qq.com%22%3Bi%3A2%3Bi%3A2592000%3Bi%3A3%3Ba%3A3%3A%7Bs%3A2%3A%22id%22%3Bs%3A4%3A%221086%22%3Bs%3A4%3A%22name%22%3Bs%3A8%3A%22Lp+Study%22%3Bs%3A4%3A%22type%22%3Bs%3A1%3A%22n%22%3B%7D%7D',
            '__utmt':'1',
            '__utma':'201260338.796552597.1428908352.1463018481.1463024893.21',
            '__utmb':'201260338.15.10.1463024893',
            '__utmc':'201260338',
            '__utmz':'201260338.1461551356.19.9.utmcsr=baidu|utmccn=(organic)|utmcmd=organic',
        }
        return cookies
    def getHeader(self):
        headers = {
            'Host': 'www.myhuiban.com',
            'Connection': 'keep-alive',
            'Cache-Control': 'max-age=0',
            'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8',
            'Upgrade-Insecure-Requests': '1',
            'User-Agent': 'Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/45.0.2454.101 Safari/537.36',
            'Accept-Encoding': 'gzip, deflate, sdch',
            'Accept-Language': 'zh-CN,zh;q=0.8',
        }
        return headers


confer_url = 'http://www.myhuiban.com/conferences'


#这是会议对象，对应某一个会议
#每一个会议对应的基本结构：
#CCF    CORE    QUALIS  简称  全称  延期  截稿日期    通知日期    会议日期    会议地点    届数  浏览
#具体会议对应的结构：
#http://www.myhuiban.com/conference/1733
#征稿： 会议描述
#录取率： 对应历年的状态
class Conference:
    #注意jsonDecoder这个函数的名字必须完全一致，传入的参数名字也必须一致
    def __init__(self, ccf, core, qualis, simple, traditinal, delay, sub, note, conf, place, num, browser, content, rate, id):
        self.ccf = ccf
        self.core = core
        self.qualis = qualis
        self.simple = simple
        self.traditinal = traditinal
        self.delay = delay
        self.sub = sub
        self.note = note
        self.conf = conf
        self.place = place
        self.num = num
        self.browser = browser
        self.content = content
        self.id = id
        self.rate = rate

    def setContent(self, content):
        self.content = content
    def setAcceptRate(self, rate):
        self.rate = rate
    def printConf(self):
        print 'CCF CORE QUALIS ShortN Long Delay SubmissionDt Notification Conference Place NumberedHold Nread'
        #print('x=%s, y=%s, z=%s' % (x, y, z))
        print self.ccf, self.core , self.qualis , self.simple , self.traditinal , self.delay , self.sub , self.note , self.conf , self.place , self.num , self.browser
    def printDetailConf(self):
        self.printConf()
        print('Content:')
        print self.content
    def isValid(self):
        return self.id == -1
    def json(self):
        pass
    @staticmethod
    def createConf(line):
        #u'<td></td><td>c</td><td>b2</td><td>IDEAL</td><td><a target="_blank" href="/conference/535">International Conference on Intelligent Data Engineering and Automated Learning</a></td><td></td><td>2016-05-15</td><td>2016-06-15</td><td>2016-10-12</td><td>Wroclaw, Poland</td><td>17</td><td>3932</td>'
        #CCF    CORE    QUALIS  简称  全称  延期  截稿日期    通知日期    会议日期    会议地点    届数  浏览
        pattern = r'<td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td><a target="_blank" href="(.*?)">(.*)</a></td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td><td>(.*?)</td>'
        info = re.findall(pattern, line, re.S)[0]

        conf = Conference()
        conf.ccf = info[0]
        conf.core = info[1]
        conf.qualis = info[2]
        conf.simple = info[3]
        conf.id = info[4]
        conf.traditinal = info[5]
        conf.delay = info[6]
        conf.sub = info[7]
        conf.note = info[8]
        conf.conf = info[9]
        conf.place = info[10]
        conf.num = info[11]
        conf.browser = info[12]
        #conf.printConf()
        return conf
class MyEncoder(json.JSONEncoder):
    def default(self,obj):
        #convert object to a dict
        d = {}
        d['__class__'] = obj.__class__.__name__
        d['__module__'] = obj.__module__
        d.update(obj.__dict__)
        return d

class MyDecoder(json.JSONDecoder):
    def __init__(self):
        json.JSONDecoder.__init__(self,object_hook=self.dict2object)
    def dict2object(self,d):
        #convert dict to object
        if'__class__' in d:
            class_name = d.pop('__class__')
            module_name = d.pop('__module__')
            module = __import__(module_name)
            class_ = getattr(module,class_name)
            args = dict((key.encode('ascii'), value) for key, value in d.items()) #get args
            #pdb.set_trace()
            inst = class_(**args) #create new instance
        else:
            inst = d
        return inst



class UrlCrawler:
    def __init__(self):
        self.rootUrl = "http://www.myhuiban.com"
        self.mgr = CookieMgr()
        self.curPage = 1
        self.endPage = -1
        self.confDic = {}
    def requestUrl(self, url):
        r = urllib2.Request(url, cookies=self.mgr.getCookie(), headers=self.mgr.getHeader())
        #r = requests.get(url, cookies=self.mgr.getCookie(), headers=self.mgr.getHeader())
        r.encoding = 'utf-8'#设置编码格式
        return r
    def setRoot(self, url):
        self.rootUrl = url
    def firstPage(self, url):
        self.pageHtml = self.requestUrl(url).text
        self.endPage = self.searchEndPage()
        self.procAPage()
    def hasNext(self):
        #pdb.set_trace()
        return self.curPage <= self.endPage
    def nextPage(self):
        #http://www.myhuiban.com/conferences?Conference_page=14
        self.pageHtml = self.requestUrl(self.rootUrl+"/conferences?Conference_page="+str(self.curPage)).text
        self.procAPage()
    def procAPage(self):
        print 'process page: ', self.curPage, ' end:', self.endPage
        pattern = r'<tbody>\n(.*?)\n</tbody>'
        #print self.pageHtml
        table = re.findall(pattern, self.pageHtml, re.S)[0]
        #pdb.set_trace()
        re_class = re.compile(r'<tr class=.*?>\n')
        table = re_class.sub('', table)#去掉span标签
        re_style=re.compile('<span.*?>',re.I)#不进行超前匹配
        table = re_style.sub('', table)
        re_style=re.compile('</span>',re.I)#去掉span
        table = re_style.sub('', table)

        table = table.split('\n')
        for i in range(len(table)):
            #print table[i]
            conf = Conference.createConf(table[i])
            self.parseConfContent(conf)
            self.confDic[conf.simple] = conf
        self.curPage += 1
    def parseConfContent(self, conf):
        html = self.requestUrl(self.rootUrl + conf.id).text
        #print html
        pattern = r'<div class="portlet-content">\n<pre>(.*?)</pre><div class'
        table = re.findall(pattern, html, re.S)[0]
        conf.setContent(table)
    def requestConf(self):
        #load first page
        self.firstPage(self.rootUrl+"/conferences")
        #print self.confDic
        while(self.hasNext()):
            self.nextPage()
    def crawlWeb(self):
        if(len(self.confDic) == 0):
            self.loadConf()
        if(len(self.confDic) == 0):
            self.requestConf()
        self.storeConf()

    def storeConf(self):
        f = file('conferences.txt', 'w')
        confJson = json.dumps(self.confDic, cls=MyEncoder)
        #print confJson
        f.write(confJson)
        f.close()
    def loadConf(self):
        f = file('conferences.txt', 'r')
        confJson = f.read()
        self.confDic = MyDecoder().decode(confJson)
        #pdb.set_trace()
        #print confJson
        f.close()

    def searchEndPage(self):
        pattern = r'<li class="last"><a href="/conferences\?Conference_page=">'
        pattern = r'<li class="last"><a href="/conferences\?Conference_page=(\d*)">'
        #print self.pageHtml
        return int(re.search(pattern, self.pageHtml).group(1))

class Pattern:
    def __init__(self):
        self.patList = list()
    def match(self, conf):

        return True
    def addPat(pat):
        self.patList.append(pat)
class OnePattern:
    def __init__(self, date, ccf, conf, key, showdetail):
        self.date = date
        self.ccf = ccf
        self.conf = conf#name
        self.key = key
        self.showdetail = showdetail

    def match(self, conf):
        res = True
        if(self.date):
            res = re.findall(self.date, conf.sub, re.S)
        if(self.conf):
            res = (re.findall(self.conf, conf.simple, re.S) or  re.findall(self.conf, conf.traditinal, re.S)) and res
        if(self.ccf):
            #print conf.ccf, self.ccf, re.findall(self.ccf, conf.ccf, re.S)
            res = re.findall(self.ccf, conf.ccf, re.S) and res
        if(self.key):
            res = re.findall(self.key, conf.content, re.S) and res


        if(res):
            if(not self.showdetail):
                conf.printConf()
            else:
                conf.printDetailConf()
            return True
        else:
            return False

class TwoOpPattern(Pattern):
    def __init__(self, a, b):
        Pattern.__init__(self)
        self.patList.append(a)
        self.patList.append(b)
class OrPattern(TwoOpPattern):
    def match(self, conf):
        return self.patList[0].match(conf) or self.patList[1].match(conf)
class AndPattern(TwoOpPattern):
    def match(self, conf):
        return self.patList[0].match(conf) and self.patList[1].match(conf)
class NotPattern(Pattern):
    def match(self, conf):
        return not self.patList[0].match(conf)


class SearchEngine:
    def __init__(self):
        self.crawler = UrlCrawler()
        self.crawler.crawlWeb()
    #date='6-8', ccf='a|b|c', conf='FAST',  key='xyz', showdetail=True
    def searchConf(self, date='2016-0[6-8]', ccf='b', name='CIDR',  key='cloud', showdetail=True):
        pat = OnePattern(date, ccf, name, key, showdetail)
        matNum = 0
        for key, value in self.crawler.confDic.items():
            if(pat.match(value)):
                matNum += 1
        print 'Total Matched Conference Num: ', matNum

if __name__ == '__main__':
    opts, args = getopt.getopt(sys.argv[1:], "hd:c:n:k:s:", ['help', 'date=', 'ccf=', 'name=',  'key=', 'showDetail='])
    date = None
    ccf = None
    name = None
    key = None
    showdetail = False
    for op, value in opts:
        if op in ("-d", '--date'):
            date = value
        elif op in ("-c", '--ccf'):
            ccf = value
        elif op in ("-n", '--name'):
            name = value
        elif op in ("-k", "--key"):
            key = value
        elif op in ("-s", "-showDetail"):
            showdetail = value
        elif op in ("-h", "--help"):
            print 'Usage:', sys.argv[0], 'options'
            print '    Mandatory arguments to long options are mandatory for short options too.'
            print '    -d, --date=DATE  search the given date, default none.'
            print '    -c, --ccf=[abc]  search the given CCF level, default none.'
            print '    -n, --name=NAME  search the given conference name(support shor and long name both).'
            print '    -k, --key=KEYWORD  search the given KEYWORD in conference description.'
            print '    -h, --help  print this help info.'
            print 'Example:'
            print '    ', sys.argv[0], '-d 2016-0[6-8] -c b -n CIDR -k cloud -s 1'
            print '        ', 'search the conference whose submission date from 2016-06 to 2016-08, level is CCF B, keyword is cloud and detail information is shown.'
            print '    ', sys.argv[0], '--name=CIDR'
            print '    ', sys.argv[0], '-n CIDR'
            print '        ', 'search the conference name which contains CIDR.'
            print '    ', sys.argv[0], '-n ^CIDR$ -s 1'
            print '        ', 'search the conference name which contains exactly CIDR and print the detail info.'
            print '    ', sys.argv[0], '-c [ab] -k "cloud computing"'
            print '        ', 'search the conference who is CCF A or B and is about cloud computing.'
            print '    ', sys.argv[0], '-k "(cloud computing) | (distributed systems)"'
            print '        ', 'search the conference which is about cloud computing or distributed systems.'
            print '    ', sys.argv[0], '-k "cloud | computing | distributed"'
            print '        ', 'search the conference which contains cloud or computing or distributed.'
            print '    ', sys.argv[0], '-k "(Cloud migration service).*(cloud computing)"'
            print '        ', 'search the conference which contains both and are ordered.'
            print
            sys.exit()
    SearchEngine().searchConf(date, ccf, name, key, showdetail)
    #print date, ccf, name, key, showdetail
    sys.exit(0)