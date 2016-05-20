#!/usr/bin/python

import web

urls = ("/webpy/.*", "hello")
app = web.application(urls, globals())

class hello: 
    def GET(self):
        return 'Hello, world!'

web.wsgi.runwsgi = lambda func, addr=None: web.wsgi.runfcgi(func, addr)
if __name__ == "__main__":
    web.webapi.internalerror = web.debugerror
    app.run()
