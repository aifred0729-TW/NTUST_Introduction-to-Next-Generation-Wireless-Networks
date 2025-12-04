#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
from datetime import datetime
from typing import Dict, Set

class ChatServer:
    def __init__(self, host='localhost', port=8888, max_connections=3):
        self.host = host
        self.port = port
        self.max_connections = max_connections
        self.server_socket = None
        self.clients: Dict[str, socket.socket] = {}
        self.client_info: Dict[str, tuple] = {}
        self.lock = threading.Lock()
        
    def get_current_time(self) -> str:
        return datetime.now().strftime("%H:%M")
    
    def log_message(self, message: str):
        timestamp = self.get_current_time()
        print(f"[{timestamp}] {message}")
    
    def broadcast_message(self, message: str, exclude_username: str = None):
        with self.lock:
            disconnected_clients = []
            for username, client_socket in self.clients.items():
                if username == exclude_username:
                    continue
                try:
                    client_socket.send(message.encode('utf-8'))
                except:
                    disconnected_clients.append(username)
            
            for username in disconnected_clients:
                self.remove_client(username)
    
    def remove_client(self, username: str):
        with self.lock:
            if username in self.clients:
                try:
                    self.clients[username].close()
                except:
                    pass
                del self.clients[username]
                if username in self.client_info:
                    del self.client_info[username]
                
                timestamp = self.get_current_time()
                leave_message = f"[{timestamp}]{username} has left the chat.\n"
                self.broadcast_message(leave_message, exclude_username=username)
                
                self.log_message(f"{username} has left the chat.")
    
    def handle_client(self, client_socket: socket.socket, address: tuple):
        username = None
        try:
            username = None
            while True:
                try:
                    data = client_socket.recv(1024).decode('utf-8')
                    if not data:
                        break
                    
                    if data.startswith("REGISTER:"):
                        proposed_username = data.split(":", 1)[1].strip()
                        
                        with self.lock:
                            if proposed_username in self.clients:
                                client_socket.send("NAME_EXISTS".encode('utf-8'))
                            else:
                                username = proposed_username
                                self.clients[username] = client_socket
                                self.client_info[username] = address
                                client_socket.send("REGISTER_SUCCESS".encode('utf-8'))
                                break
                    else:
                        client_socket.close()
                        return
                        
                except Exception as e:
                    self.log_message(f"Error receiving username: {e}")
                    break
            
            if not username:
                return
            
            while True:
                try:
                    data = client_socket.recv(1024).decode('utf-8')
                    if not data:
                        break
                    
                    if data.strip().lower() == "exit":
                        timestamp = self.get_current_time()
                        leave_message = f"[{timestamp}]{username} has left the chat.\n"
                        self.broadcast_message(leave_message, exclude_username=username)
                        self.log_message(f"{username} has left the chat.")
                        client_socket.send("EXIT_SUCCESS".encode('utf-8'))
                        with self.lock:
                            if username in self.clients:
                                del self.clients[username]
                            if username in self.client_info:
                                del self.client_info[username]
                        break
                    
                    timestamp = self.get_current_time()
                    message = f"[{timestamp}][{username}]: {data}\n"
                    self.broadcast_message(message, exclude_username=username)
                    
                except Exception as e:
                    self.log_message(f"Error handling message from {username}: {e}")
                    break
                    
        except Exception as e:
            self.log_message(f"Error in client handler: {e}")
        finally:
            if username:
                with self.lock:
                    if username in self.clients:
                        self.remove_client(username)
                try:
                    client_socket.close()
                except:
                    pass
    
    def start(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        
        try:
            self.server_socket.bind((self.host, self.port))
            self.server_socket.listen(5)
            self.log_message(f"Server started on {self.host}:{self.port}")
            self.log_message(f"Maximum connections: {self.max_connections}")
            
            while True:
                client_socket, address = self.server_socket.accept()
                
                with self.lock:
                    if len(self.clients) >= self.max_connections:
                        self.log_message(f"Connection attempt from {address} rejected: Maximum connections reached.")
                        client_socket.send("MAX_CONNECTIONS_REACHED".encode('utf-8'))
                        client_socket.close()
                    else:
                        self.log_message(f"New connection from {address}")
                        client_thread = threading.Thread(
                            target=self.handle_client,
                            args=(client_socket, address)
                        )
                        client_thread.daemon = True
                        client_thread.start()
                
        except KeyboardInterrupt:
            self.log_message("Server shutting down...")
        except Exception as e:
            self.log_message(f"Server error: {e}")
        finally:
            if self.server_socket:
                self.server_socket.close()
            with self.lock:
                for client_socket in self.clients.values():
                    try:
                        client_socket.close()
                    except:
                        pass

if __name__ == "__main__":
    server = ChatServer(host='localhost', port=8888, max_connections=3)
    server.start()
