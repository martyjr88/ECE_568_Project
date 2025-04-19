import network
import urequests
import time

wlan = network.WLAN(network.STA_IF)
wlan.active(True)
if not wlan.isconnected():
    print("connecting to network...")
    SSID = "Weida Apt 1-444 N Grant St"
    Password = "NmvcSPgcwVm!"
    wlan.connect(SSID, Password)
    while not wlan.isconnected():
        pass
    print(f"Connected to {SSID}")
    print(f"IP Address: {wlan.ifconfig()[0]}")

user_id = "user1"
server_ip = "192.168.5.47"
url = f"http://{server_ip}:8000/submit"

# Send the POST request
headers = {"Content-Type": "application/x-www-form-urlencoded"}
payload = f"key={user_id}"

try:
    response = urequests.post(url, data=payload, headers=headers)
    print("Response:", response.text)
    response.close()
except Exception as e:
    print("Error sending request:", e)
