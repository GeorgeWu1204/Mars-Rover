# import socket
# import random
# import time
# from threading import Thread
# import os
# from _thread import *

# def receive(connection_socket, webserver_msg):
#     while True:
#         if (webserver_msg[0] != "end"):
#             cmsg = connection_socket.recv(1024)
#             cmsg = cmsg.decode()
#             print("received: ", cmsg)


# def send(connection_socket,webserver_msg):
#     time.sleep(1)
#     start_msg = "!start$"
#     connection_socket.send(start_msg.encode())
#     print("Start message sent----------------------------------------------")

#     time.sleep(10)

#     end_msg = "!end$"
#     connection_socket.send(end_msg.encode())
#     print("End message sent--------------------------------------------------")

#     webserver_msg[0] = "end"
    

# print("We're in tcp server...")
# server_port = 12000
# welcome_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# welcome_socket.bind(('0.0.0.0',server_port))
# welcome_socket.listen(1)
# print('Hello, Server running on port ', server_port)
# connection_socket, caddr = welcome_socket.accept()
# cmsg = connection_socket.recv(1024)
# cmsg = cmsg.decode()
# print("first message received:", cmsg)
# print("ESP32 Client connected")

# webserver_msg = [""]

# thread = Thread (target=receive,args =(connection_socket,webserver_msg,))
# thread.start()

# send(connection_socket,webserver_msg)

