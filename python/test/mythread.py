#!/usr/bin/env python
# -*- coding: utf-8

import os
from multiprocessing import Process


def run_proc(name, info):
    print 'process %s' % name, info, os.getpid()

if __name__ == "__main__":
    print "main thread = %s" % os.getpid()
    p = Process(target=run_proc, args={"test", "12123"})
    p.start()
    p.join()
    print "end  process"