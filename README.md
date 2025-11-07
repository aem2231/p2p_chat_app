# P2P Chat App

A terminal-based peer-to-peer chat application built with C++, Boost.Asio, and FTXUI.

Do note that this is passion project. It only supports peer discovery and chatting over a local network at the moment. If you do want to use it, you need to open TCP port 9000 and UDP port 9001 for the time being.

## Building

I used [Nix flakes](https://nixos.wiki/wiki/Flakes) to manage dependencies and my dev environment for this project.

1.  **Enter the development environment:**

    ```sh
    nix develop
    ```

    **Or using direnv:**
    ```sh
    direnv allow
    ```

2.  **Build the project:**
    ```sh
    make
    ```

3.  **Run the app:**
    ```sh
    ./bin/chat
    ```

## I plan to add the following features in the future.
- End-to-end Encryption
- DHT for peer discovery over the internet
- NAT Traversal
- Friends List
- Group Chats
- Voice Messages
- Voice Calls
