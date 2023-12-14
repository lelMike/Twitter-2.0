# Twitter-2.0

## Overview
Twitter-2.0 is a terminal-based Twitter clone, developed for training purposes. The project utilizes Inter-Process Communication (IPC) System V, specifically employing semaphores and shared memory. Please note that the code has been tested on Linux (Ubuntu) only and is not compatible with Windows.

## Getting Started

### Prerequisites
- Linux environment (tested on Ubuntu)
- GCC compiler

### Building the Executables
Compile the server and client files using the following commands:

```bash
gcc -o server mini_twitter_2_0_server.c
gcc -o client mini_twitter_2_0_client.c
```

Note: Make sure you have the necessary prerequisites installed before compiling.

### Running the Server
Start the server with the following command:

```bash
./server <path> <num_posts>
```

- path: The path to initialize shared memory and semaphores.
- num_posts: The number of initial posts to configure.
After initialization, you can press Ctrl + Z to display all posts and likes with their authors. Use Ctrl + C to detach, clean up shared memory and semaphores, and terminate the server.

### Running the Client
Execute the client program with the following command:

```bash
./client <path> <username>
```

- path: The same path used for server initialization.
- username: Your desired username to be displayed as the author of your posts.
Interacting with the Client
Press 'N' to create a new post.
Press 'L' to like a post. Enter the post number in STDIN when prompted.

### Additional Information
The code may be further expanded to enhance functionality and implement individual semaphores per post.
For any questions or clarifications, please feel free to reach out.

Note: This project is intended for educational purposes, and usage is recommended in a controlled environment.
