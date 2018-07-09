#!/usr/bin/env python
# -*- coding: utf-8

from ctypes import *
import re
import json


#data = """{u'pc_authorize_h5flag': 1, u'pc_authorize_size': 1000, u'pc_authorize_so': 1, u'pc_authorize_type': 1, u'result': 1001, u'pc_authorize_logflag': 1, u'pc_authorize_packnum': 2000, u'pc_authorize_localpack': 1}"""
#data = """{"appName":"新浪财经","appStatus":3,"createTime":"2017-12-25 17:47:06","appSize":24012027,"appVersion":"4.1.5","appAlias":"新浪财经_","appBackName":"新浪财经_.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20170816095509181\/icon.png","aid":117017,"isSafe":1,"isunpack":1}"""
data = """[{"appName":"途牛旅游","appStatus":3,"createTime":"2017-11-04 17:48:52","appSize":40156983,"appVersion":"9.1.4","appAlias":"途牛旅游","appBackName":"途牛旅游.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911182314740\/icon.png","aid":113252,"isSafe":1,"isunpack":1},{"appName":"途牛旅游","appStatus":3,"createTime":"2017-11-04 17:46:06","appSize":40156983,"appVersion":"9.1.4","appAlias":"途牛旅游","appBackName":"途牛旅游.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911182314740\/icon.png","aid":113250,"isSafe":1,"isunpack":1},{"appName":"新浪财经","appStatus":3,"createTime":"2017-11-04 17:42:32","appSize":24012027,"appVersion":"4.1.5","appAlias":"新浪财经_","appBackName":"新浪财经_.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20170816095509181\/icon.png","aid":113249,"isSafe":1,"isunpack":1},{"appName":"139邮箱","appStatus":3,"createTime":"2017-11-04 17:30:25","appSize":25159921,"appVersion":"7.2.2","appAlias":"139邮箱","appBackName":"139邮箱.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20171027141208162\/icon.png","aid":113245,"isSafe":1,"isunpack":1},{"appName":"途牛旅游","appStatus":3,"createTime":"2017-11-04 17:29:26","appSize":40156983,"appVersion":"9.1.4","appAlias":"途牛旅游","appBackName":"途牛旅游.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911182314740\/icon.png","aid":113244,"isSafe":1,"isunpack":1},{"appName":"途牛旅游","appStatus":3,"createTime":"2017-11-04 17:25:30","appSize":40156983,"appVersion":"9.1.4","appAlias":"途牛旅游","appBackName":"途牛旅游.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911182314740\/icon.png","aid":113242,"isSafe":1,"isunpack":1},{"appName":"途牛旅游","appStatus":3,"createTime":"2017-11-04 17:07:13","appSize":40156983,"appVersion":"9.1.4","appAlias":"途牛旅游","appBackName":"途牛旅游.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911182314740\/icon.png","aid":113236,"isSafe":1,"isunpack":1},{"appName":"新浪财经","appStatus":3,"createTime":"2017-11-04 17:05:43","appSize":24012027,"appVersion":"4.1.5","appAlias":"新浪财经_","appBackName":"新浪财经_.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20170816095509181\/icon.png","aid":113235,"isSafe":1,"isunpack":1},{"appName":"signaturetest","appStatus":3,"createTime":"2017-11-04 11:09:07","appSize":840753,"appVersion":"1.0","appAlias":"v2","appBackName":"v2.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pcapk\/apkCache\/20170911180747895\/icon.png","aid":113189,"isSafe":1,"isunpack":0},{"appName":"新浪财经","appStatus":3,"createTime":"2017-10-31 15:10:37","appSize":24012027,"appVersion":"4.1.5","appAlias":"新浪财经_","appBackName":"新浪财经_.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20170816095509181\/icon.png","aid":112555,"isSafe":1,"isunpack":0},{"appName":"139邮箱","appStatus":3,"createTime":"2017-10-31 15:10:38","appSize":25159921,"appVersion":"7.2.2","appAlias":"139邮箱","appBackName":"139邮箱.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20171027141208162\/icon.png","aid":112556,"isSafe":1,"isunpack":0}]"""

if __name__ == "__main__":
    data_json = json.loads(data)
    if isinstance(data_json, dict):
        print(data_json)
    if isinstance(data_json, list):
        for line in data_json:
            print(line)
    else:
        pass