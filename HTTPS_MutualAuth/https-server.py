#!/usr/bin/env python3

import http.server
import ssl


def get_ssl_context(certfile, keyfile):
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile, keyfile)
    context.set_ciphers("@SECLEVEL=1:ALL")
    context.load_verify_locations(cafile="certs/rootca-cert.pem")
    context.verify_mode = ssl.CERT_REQUIRED
    # context.verify_mode = ssl.CERT_OPTIONAL
    return context


class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        content_length = int(self.headers["Content-Length"])
        post_data = self.rfile.read(content_length)
        print(post_data.decode("utf-8"))


server_address = ('', 5000)
httpd = http.server.HTTPServer(server_address, MyHandler)

context = get_ssl_context("certs/server-cert.pem", "certs/server-key.pem")
httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

httpd.serve_forever()