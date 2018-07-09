#!/usr/bin/env python
# -*- coding: utf-8

from ctypes import *
import re
import json

strData = """{"appName":"新浪财经","appStatus":3,"createTime":"2017-12-25 17:47:06","appSize":24012027,"appVersion":"4.1.5","appAlias":"新浪财经_","appBackName":"新浪财经_.apk","appLogo":"http:\/\/fs8.ijiami.cn\/ijiami\/pctestapk\/apkCache\/20170816095509181\/icon.png","aid":117017,"isSafe":1,"isunpack":1}"""

class Fileoper(object):
    len  = 0
    file = ''
    info = ''
    def __init__(self):
        self.file = "b"
        self.info = "a"
        self.len = 0
        pass

    def getinfo(self):
        print self.file

    def __str__(self):
        return "this is test info [%s]" % self.file

    def getlen(self):
        print self.len

    def is_json(self,myjson):
        try:
            json.loads(myjson)
        except ValueError:
            return False
        return True

    def pasfile(self, filename):
        self.file = filename
        fileinfo = open(filename, "rb")
        for line in fileinfo:
            if(len(line) > 0 ):
                self.formatinfo(line)

        fileinfo.close()

    def formatinfo(self, str):
        if True == self.is_json(str):
            json_str = json.loads(str)
            if isinstance(json_str, dict):
                self.resolve_unit( json_str)
            else:
                if isinstance(json_str, list):
                    for json_item in json_str:
                        self.resolve_unit(json_item)
                pass


    def resolve_unit( self, str):
        for key in str:
            value = "%s: %s"%(key,str[key])
            print(value)
        pass




if __name__ == "__main__":
    op = Fileoper()
    op.pasfile("E:\\code\\python\\temp.log")
    pass
