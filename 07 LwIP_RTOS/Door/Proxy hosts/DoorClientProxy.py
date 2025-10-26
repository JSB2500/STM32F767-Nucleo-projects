import socket
import sys
from threading import *
import time

# ServerIP = "192.168.1.13" # JSB local test server.
ServerIP = "151.80.60.118" # JSB remote OVH test server(OVH).
ServerPort = 18753

Locked = False

try:
	ClientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
except socket.error:
	print('Failed to create socket.')
	sys.exit()

print('Socket created.')

try:
	remote_ip = socket.gethostbyname(ServerIP)
except socket.gaierror:
	print('Hostname could not be resolved.')
	sys.exit()

while True:    
    try:
        ClientSocket.connect((ServerIP, ServerPort))
        break
    except:
        print('Failed to connect.')
        time.sleep(2)
    
print('Socket connected to ' + ServerIP + ' on ip ' + remote_ip)

message = "GET /arduino HTTP/1.1\r\n\r\n"
try :
    print('Sending: ' + message)
    ClientSocket.send(message.encode())
except socket.error:
	print('Send failed.')
	sys.exit()

while True:
    Ch = ClientSocket.recv(1).decode()
    print('Received: '+Ch)
    if (Ch=='p'):
        print("Command: Ping.")
        if Locked:
            S = 'c'
        else:
            S = 'o'
        print('Sending: '+S)
        ClientSocket.send(S.encode())
    elif (Ch=='o'):
        print("Command: Open.")
        Locked = not Locked
        
        if Locked:
            S = 'Locked'
        else:
            S = 'Unlocked'
            
        print("Door locked status: " + S)
    
ClientSocket.close()

