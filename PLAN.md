### **Part 1: UI & Simulated Discovery**

*   **Peer Object (`include/network/peer.hpp`, `src/network/peer.cpp`)**
    *   A data class representing a single user.
    *   Fields: hostname, network address.

*   **Discovery Class (`include/network/discovery.hpp`, `src/network/discovery.cpp`)**
    *   A placeholder for network discovery.
    *   A `getPeers()` method returns a hardcoded list of `Peer` objects for UI testing.

*   **App Class (`include/core/app.hpp`, `src/core/app.cpp`)**
    *   The client's central logic controller.
    *   Fetches the peer list from the `Discovery` object.
    *   Manages state, such as the currently selected peer.

*   **PeerList UI Component (`include/ui/peer_list.hpp`, `src/ui/peer_list.cpp`)**
    *   Renders the list of peers.
    *   Receives data from the `App` class.
    *   Handles UI logic for peer selection.

*   **ChatWindow UI Component (`include/ui/chat_window.hpp`, `src/ui/chat_window.cpp`)**
    *   The main chat interface.
    *   Initially, a placeholder showing the selected peer's name and an input box.

*   **Connection Class (`include/network/connection.hpp`, `src/network/connection.cpp`)**
    *   A placeholder for a direct TCP connection.
    *   Outlined with empty methods: `connect()`, `sendMessage()`, `receiveMessage()`.

*   **Crypto Class (`include/crypto/crypto.hpp`, `src/crypto/crypto.cpp`)**
    *   A placeholder for encryption logic.
    *   `encrypt()` and `decrypt()` methods that initially pass data through unmodified.

*   **Main Entry Point (`src/main.cpp`)**
    *   Initializes all components.
    *   Constructs the final UI layout from the `PeerList` and `ChatWindow`.
    *   Runs the main FTXUI event loop.

### **Part 2: Real P2P Messaging**

*   **Listener in App Class (`src/core/app.cpp`)**
    *   A background thread, launched by `App`, to listen for incoming peer connections using Boost.Asio.

*   **Networking in Connection Class (`src/network/connection.cpp`)**
    *   Full implementation of `connect`, `sendMessage`, and asynchronous `receiveMessage` methods using Boost.Asio for TCP communication.

*   **Message Handling in App Class (`src/core/app.cpp`)**
    *   Manages active `Connection` objects.
    *   Routes outgoing messages from the UI to the correct `Connection`.
    *   Processes incoming messages from `Connection` objects.

*   **Dynamic ChatWindow (`src/ui/chat_window.cpp`)**
    *   Displays a live, scrolling history of messages.
    *   Sends user input to the `App` class for network delivery.
    *   Updates its display when new messages are received from the `App`.

*   **Thread-Safe UI Updates (`src/main.cpp`)**
    *   A mechanism (e.g., FTXUI's `PostEvent`) for the background network thread to safely trigger a screen refresh on the main UI thread, making incoming messages appear instantly.
