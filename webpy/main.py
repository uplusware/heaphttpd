#!/usr/bin/python

import web
import string
urls = ("/webpy/.*", "hello")
app = web.application(urls, globals())

class hello: 
    def GET(self):
        response_body = '<h1>Web.py DEMO</h1><hr>Hello, GET based on web.py/spawn-fcgi!'
        web.header('Content-Length', '%d' % len(response_body));
        return response_body
    def POST(self):
        i = web.input();
        response_body = '<h1>Web.py DEMO</h1><hr>Hello, POST based on web.py/spawn-fcgi! ' + i.abcd + " + " + i.efgh + " = " + str((string.atoi(i.abcd) + string.atoi(i.efgh)))
        web.header('Content-Length', '%d' % len(response_body));
        return response_body
        
web.wsgi.runwsgi = lambda func, addr=None: web.wsgi.runfcgi(func, addr)
if __name__ == "__main__":
    app.run()
