import http.server
import socketserver
import os

PORT = 8000

class OtaRequestHandler(http.server.SimpleHTTPRequestHandler):
    def do_POST(self):
        # Log the detailed request (optional)
        print(f"Received POST request from {self.client_address[0]}")
        
        # In a real scenario, you might read the body:
        # content_length = int(self.headers['Content-Length'])
        # body = self.rfile.read(content_length)
        # print("Body:", body.decode('utf-8'))
        
        # For this simple static server, simply handle it as a GET
        # to return the version.json file requested.
        self.do_GET()

if __name__ == "__main__":
    # Ensure we serve from the current directory
    print(f"Serving HTTP on 0.0.0.0 port {PORT} (POST supported) ...")
    with socketserver.TCPServer(("", PORT), OtaRequestHandler) as httpd:
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\nStopping server.")
            httpd.server_close()
