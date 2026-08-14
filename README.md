# CS3205 – Assignment 2  
## Multi-Client Chat System with Performance Analytics



# 1. System Architecture Overview

## 1.1 Introduction

This project implements a multi-client chat application with the following features:

- Broadcast messaging
- Private messaging
- User authentication
- Active user management
- Centralized chat history storage (JSON format)
- DNS-like discovery service
- Performance benchmarking and monitoring

The system is designed to compare different concurrency models and evaluate their performance under load and stress conditions.



## 1.2 System Components

### 1. Discovery Server

The Discovery Server acts as a DNS-like service for the chat system.

Responsibilities:
- Stores user credentials (username/password)
- Handles user signup and login verification
- Registers the chat server (IP and port)
- Provides chat server details to authenticated users

It ensures that clients can dynamically locate the chat server.



### 2. Chat Server

Three different implementations of the Chat Server are provided:

1. Fork-based server (process-based concurrency)
2. Thread-based server (pthreads)
3. Select-based server (non-blocking I/O)

All versions support:

- Broadcast messaging
- Private messaging
- Active user list
- Graceful disconnection
- Chat history storage in JSON format
- Performance monitoring (CPU, VmRSS, PSS)

The concurrency models allow comparison of scalability and resource usage.



### 3. Chat Client

The Chat Client:

- Authenticates via the discovery server
- Connects to the chat server
- Supports:
  - Broadcast messages
  - Private messages
  - Viewing online users (LIST)
  - Viewing personal chat history (HISTORY)
  - EXIT
- Measures message delivery latency using timestamps

Latency is calculated as:

Latency = Receive Time - Send Time



# 2. Protocol Specification

## 2.1 Message Framing

All communication uses length-prefixed framing:

[4-byte length][message payload]

This ensures proper handling of variable-length messages and prevents message boundary issues.



## 2.2 Discovery Server Protocol

CHECK_USER <username>  
SIGNUP <username> <password>  
VERIFY <username> <password>  
REGISTER_CHAT_SERVER <ip> <port>  

Responses include:

USER_NOT_FOUND  
USER_EXISTS  
SIGNUP_SUCCESS  
WRONG_PASSWORD  
LOGIN_SUCCESS <ip> <port>  
CHAT_SERVER_REGISTERED  



## 2.3 Chat Server Protocol

Broadcast:

BROADCAST|<timestamp>|<message>

Private Message:

PRIVATE|<receiver>|<timestamp>|<message>

List Online Users:

LIST

View History:

HISTORY

Exit:

EXIT



## 2.4 Server Responses

Broadcast received:

MESSAGE|<sender>|<timestamp>|<message>

Private received:

PRIVATE|<sender>|<timestamp>|<message>

Online users list:

ONLINE|user1,user2,user3



# 3. Compilation Instructions

Compile all components:

make

Clean all binaries and logs:

make clean

Rebuild:

make rebuild



# 4. Execution Instructions

## Step 1 – Start Discovery Server

./discovery

## Step 2 – Start Chat Server (Choose One)

./server_fork  
or  
./server_thread  
or  
./server_select  

## Step 3 – Start Client

./client



# 5. Load Testing

Simulates a fixed number of concurrent clients (e.g., 10).

Example:

./run_test.sh select 10 60

This runs 10 clients for 60 seconds.

Generated files:

- select_latency.txt
- select_metrics.txt



# 6. Stress Testing

Gradually increases number of clients.

Run:

./run_stress_test.sh

This generates:

- fork_summary.txt
- thread_summary.txt
- select_summary.txt
- Performance plots (.png files)



# 7. Author

Roll Number: <CS23B043>  
Course: CS3205  
Assignment: Multi-Client Chat System with Performance Analytics