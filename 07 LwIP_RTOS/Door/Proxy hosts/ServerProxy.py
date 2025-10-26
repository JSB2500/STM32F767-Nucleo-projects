import socket
import sys
from threading import *
import time

HOST = ''
PORT = 18753

Locked = False

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print('Socket created.')

try:
        s.bind((HOST, PORT))
except (socket.error , msg):
        print('Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1])
        sys.exit()

print('Socket bind complete.')

s.listen(10)
print('Socket listening.')

while 1:
        conn, addr = s.accept()

        print('Connected with ' + addr[0] + ':' + str(addr[1]))

        ReceivedData = conn.recv(1024).decode();
        print('Received data: '+ReceivedData)
        if ReceivedData!="GET /arduino HTTP/1.1\r\n\r\n":
            print('Arduino not found.')
            conn.close()
        else:
            print('Arduino found.')
            while True:
                message='p'
                try:
                    conn.send(message.encode())
                except ConnectionResetError:
                    print('Connection reset.')
                    break;
                print('Ping command sent.')
                ReceivedData = conn.recv(128).decode()
                print('Received data: '+ReceivedData)
                if (ReceivedData==''): # 0 always means closed connection.
                    print('Connection lost. [1]')
                    break;
                if ReceivedData=='o':
                    Locked = False
                elif ReceivedData=='c':
                    Locked = True
                else:
                    print('Invalid response ('+ReceivedData+') from client.')
                print('Locked: ' + str(Locked))
                
                time.sleep(2)

            conn.close()

s.close()


