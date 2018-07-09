#!C:\Python27\python.exe
# -*- coding:UTF-8 -*-

import Tkinter

def center_window(w = 300, h = 200):
    ws = root.winfo_screenwidth()
    hs = root.winfo_screenheight()
    x = (ws/2) - (w/2)
    y = (hs/2) - (h/2)
    root.geometry("%dx%d+%d+%d" % (w, h, x, y))

root = Tkinter.Tk(className='python gui')
center_window(500, 300)
root.mainloop()