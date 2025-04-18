from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
from datetime import datetime

VALID_KEYS = {'user1', 'user2', 'user3'}
login_attempts = []

class LoginRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        """Handle GET requests."""
        # Parse the URL path and query string
        parsed_path = urlparse(self.path)
        if parsed_path.path in ["/", "/logs"]:
            # If the root or /logs is requested, serve the log page
            self.send_response(200)
            self.send_header("Content-Type", "text/html")
            self.end_headers()
            # Build an HTML page showing the log table
            html = "<html><head><title>Login Attempts Log</title></head><body>"
            html += "<h1>Login Attempts</h1>"
            html += "<table border='1'><tr><th>Timestamp</th><th>User Key</th><th>Status</th></tr>"
            for entry in login_attempts:
                ts, key, status = entry
                html += f"<tr><td>{ts}</td><td>{key}</td><td>{status}</td></tr>"
            html += "</table></body></html>"
            self.wfile.write(html.encode("utf-8"))
        elif parsed_path.path == "/submit":
            # This is the endpoint for ESP32 to submit a key, e.g. /submit?key=user119
            query_params = parse_qs(parsed_path.query)
            if "key" not in query_params:
                # No key provided – bad request
                self.send_response(400)
                self.end_headers()
                self.wfile.write(b"Missing 'key' parameter")
                return
            # Extract the key value
            key = query_params["key"][0]
            # Check against valid keys
            status = "Success" if key in VALID_KEYS else "Failure"
            # Log the attempt with current timestamp
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            login_attempts.append((timestamp, key, status))
            # Respond with a simple text acknowledgment (no specific content needed for ESP32)
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"Logged")
        else:
            # Unknown path
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            
    def do_POST(self):
        """Handle POST requests (alternative way to submit the key)."""
        parsed_path = urlparse(self.path)
        if parsed_path.path == "/submit":
            # Read the length of the body and then the body data
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length)
            # Assume the body contains just the key or a form like "key=user119"
            body_text = body.decode("utf-8").strip()
            if "=" in body_text:
                # If the key is sent as a form field like key=user119
                params = parse_qs(body_text)
                key = params.get("key", [""])[0]
            else:
                # If the body is just the raw key
                key = body_text
            # Determine success or failure
            status = "Success" if key in VALID_KEYS else "Failure"
            timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
            login_attempts.append((timestamp, key, status))
            # Respond with a plain acknowledgment
            self.send_response(200)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(b"Logged")
        else:
            self.send_response(404)
            self.end_headers()
            self.wfile.write(b"Not Found")
            
# Set up and start the HTTP server
server_address = ("0.0.0.0", 8000)  # listen on all network interfaces, port 8000
httpd = HTTPServer(server_address, LoginRequestHandler)
print(f"Serving HTTP on {server_address[0]} port {server_address[1]}")
httpd.serve_forever()
