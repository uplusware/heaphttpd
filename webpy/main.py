#!/usr/bin/python

import web
import string
urls = ("/webpy/.*", "hello")
app = web.application(urls, globals())

class hello: 
    def GET(self):
        return 'Hello, GET!'
    def POST(self):
        i = web.input();
        return 'Hello, POST! ' + i.abcd + " + " + i.efgh + " = " + str((string.atoi(i.abcd) + string.atoi(i.efgh)))

web.wsgi.runwsgi = lambda func, addr=None: web.wsgi.runfcgi(func, addr)
if __name__ == "__main__":
    app.run()
