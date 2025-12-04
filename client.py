#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
import sys
import time

class ChatClient:
    def __init__(self, host='localhost', port=8888):
        self.host = host
        self.port = port
        self.socket = None
        self.username = None
        self.running = False
        self.exit_received = False
        
    def connect(self):
        try:
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.socket.connect((self.host, self.port))
            self.socket.settimeout(1.0)
            try:
                response = self.socket.recv(1024).decode('utf-8')
                if response == "MAX_CONNECTIONS_REACHED":
                    print("Unable to connect to server: Maximum connections reached.")
                    self.socket.close()
                    sys.exit(0)
            except socket.timeout:
                pass
            except:
                pass
            self.socket.settimeout(None)
            self.running = True
            return True
        except Exception as e:
            print(f"Unable to connect to server: {e}")
            return False
    
    def register_username(self):
        while True:
            username = input("Enter your username: ").strip()
            if not username:
                print("Username cannot be empty. Please try again.")
                continue
            
            try:
                self.socket.send(f"REGISTER:{username}".encode('utf-8'))
                
                response = self.socket.recv(1024).decode('utf-8')
                
                if response == "NAME_EXISTS":
                    print("Name already in use. Please enter a different name.")
                    continue
                elif response == "REGISTER_SUCCESS":
                    self.username = username
                    print(f"Welcome, {username}! You can start chatting now.")
                    return True
                elif response == "MAX_CONNECTIONS_REACHED":
                    print("Unable to connect to server: Maximum connections reached.")
                    sys.exit(0)
                else:
                    print(f"Unexpected response from server: {response}")
                    return False
                    
            except Exception as e:
                print(f"Error registering username: {e}")
                return False
    
    def receive_messages(self):
        while self.running:
            try:
                data = self.socket.recv(1024).decode('utf-8')
                if not data:
                    break
                
                if data == "EXIT_SUCCESS":
                    print("\nYou left the chat. Disconnected from server.")
                    self.exit_received = True
                    self.running = False
                    break
                
                print(data, end='')
                
            except Exception as e:
                if self.running:
                    print(f"\nConnection error: {e}")
                break
        
        self.running = False
    
    def send_message(self, message: str):
        try:
            self.socket.send(message.encode('utf-8'))
        except Exception as e:
            print(f"Error sending message: {e}")
            self.running = False
    
    def start(self):
        if not self.connect():
            return
        
        if not self.register_username():
            self.socket.close()
            return
        
        receive_thread = threading.Thread(target=self.receive_messages)
        receive_thread.daemon = True
        receive_thread.start()
        
        try:
            while self.running:
                user_input = input()
                
                if not self.running:
                    break
                
                if user_input.strip().lower() == "exit":
                    self.send_message("exit")
                    timeout = 0
                    while not self.exit_received and timeout < 50:
                        time.sleep(0.1)
                        timeout += 1
                    if not self.exit_received:
                        print("\nYou left the chat. Disconnected from server.")
                    break
                elif user_input.strip():
                    self.send_message(user_input)
                    
        except KeyboardInterrupt:
            print("\nInterrupted by user")
            self.send_message("exit")
            time.sleep(0.5)
        except EOFError:
            self.send_message("exit")
            time.sleep(0.5)
        finally:
            if not self.exit_received:
                time.sleep(0.3)
            self.running = False
            if self.socket:
                try:
                    self.socket.close()
                except:
                    pass

if __name__ == "__main__":
    client = ChatClient(host='localhost', port=8888)
    client.start()
