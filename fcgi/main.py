#!/usr/bin/python

import web
import string
urls = ("/fcgi/.*", "hello")
app = web.application(urls, globals())

class hello: 
    def GET(self):
        response_body = '<h1>Fast-CGI DEMO</h1><hr>Hello, GET based on fastcgi!'
        web.header('Content-Length', '%d' % len(response_body));
        return response_body
    def POST(self):
        i = web.input();
        response_body = '<h1>Fast-CGI DEMO</h1><hr>Hello, POST based on fastcgi! ' + i.abcd + " + " + i.efgh + " = " + str((string.atoi(i.abcd) + string.atoi(i.efgh)))
        web.header('Content-Length', '%d' % len(response_body));
        return response_body

web.wsgi.runwsgi = lambda func, addr=("127.0.0.1", 9003): web.wsgi.runfcgi(func, addr)
if __name__ == "__main__":
    app.run()
