# **P2P Chat App - Detailed Plan**

---

## **Overall Progress**

- [✅] **Part 1: Project Foundation & UI Shell**
- [📍] **Part 2: Core P2P Messaging Logic**
- [ ] **Part 3: Real-Time UI Integration**
- [ ] **Part 4: Advanced Features & Polish**

---

## **Part 2: Core P2P Messaging Logic (Complete)**

### **`✅` Module 1: Connection Class**
- Implemented `Connection` class to manage a single TCP socket.
- Handles connecting, sending, and receiving.
- `receiveLoop` runs on a background thread.

### **`✅` Module 2: App Connection Management**
- `App` class now manages a map of `Peer` to `Connection` objects.
- `connectToPeer` and `sendMessageToSelected` are implemented.
- `onMessageReceived` callback pushes incoming messages to a thread-safe queue.

### **`✅` Module 3: Listener & Handshake**
- `App` starts a `listenerLoop` on a background thread.
- `tcp::acceptor` listens for incoming connections on port 9000.
- A simple handshake (`HELLO|hostname|ip`) identifies connecting peers.

---

## **Part 3: Real-Time UI Integration (Current Focus)**

### **`📍` Module 4: Displaying Messages in the UI**
- **Goal:** Replace the placeholder in `ChatWindow` with the actual conversation history.
- **File:** `src/ui/chat_window.cpp`
- **Steps:**
    1.  In the `ChatWindow` renderer lambda, get the currently selected peer from `App`.
    2.  If a peer is selected, call `app_->getMessageHistory(selected_peer)` to get the messages.
    3.  Create a `ftxui::Elements` vector to hold the UI representation of each message.
    4.  Loop through the `std::vector<Message>`:
        - For each `Message`, format it into a nice string (e.g., `"Eris (14:32:05): Hello there!"`). Use `message.getFormattedTime()`.
        - Create an `ftxui::text` element for the formatted string.
        - Add the element to your `Elements` vector.
    5.  Use `ftxui::vbox(elements)` to stack the messages vertically.
    6.  Make the message area scrollable by applying `| vscroll_indicator | frame` to the `vbox`.
- **File:** `src/main.cpp`
- **Steps:**
    1.  The `App` already queues incoming messages. We need to tell the UI when to refresh.
    2.  Modify the main loop (`screen.Loop(layout)`) to be a custom loop.
    3.  Inside the loop, call `app.pollIncomingMessages()` periodically (e.g., every 100ms).
    4.  If `pollIncomingMessages` returns any messages, it means we need to redraw the screen. Call `screen.PostEvent(ftxui::Event::Custom)`. This tells FTXUI to re-render everything.

### **`⬜️` Module 5: Sending Messages from the UI (Already Done!)**
- **Goal:** Hook up the input box to send a message on 'Enter'.
- **Status:** You've already implemented this in `ChatWindow::ChatWindow`! The `CatchEvent` handler calls `app_->sendMessageToSelected`. Great job getting ahead!

---

## **Part 4: Advanced Features & Polish**

### **`⬜️` Module 6: Connection Status & Polish**
- **Goal:** Make the UI more informative and robust.
- **Steps:**
    1.  **Peer List Status:**
        - In `PeerList::getComponent`, modify the renderer.
        - For each peer in the list, call `app_->isConnectedTo(peer)`.
        - Prepend the peer name with a visual indicator: `"[●] eris"` for connected, `"[ ] root"` for disconnected. You can use `ftxui::text` with different colors.
    2.  **Error Handling:**
        - In `App::connectToPeer`, the `try-catch` block currently just prints to `std::cerr`.
        - We can add a new member to `App`, maybe `std::string last_error_`, to store user-facing error messages.
        - In `ChatWindow`, display this error message somewhere.
    3.  **Auto-Connect:**
        - In `ChatWindow::ChatWindow`, when the user presses Enter, check if `app_->isConnectedTo(selected_peer)` is false.
        - If it's not connected, first call `app_->connectToPeer(selected_peer)` before sending the message.
    4.  **Graceful Shutdown:**
        - In `App::~App`, ensure `listener_thread_` is properly joined.
        - Add a destructor to `App` that sets `listening_ = false`, closes the `acceptor_`, and joins the thread. This prevents the app from crashing on exit.

### **`⬜️` Module 7: Real Peer Discovery (UDP Broadcast)**
- **Goal:** Replace the hardcoded peer list with automatic discovery on the local network.
- **Concept:** We'll use UDP broadcasting. One instance sends out a "Who's here?" message to everyone on the network, and other instances reply directly to the sender.
- **Steps:**
    1.  **Discovery Service Class (`src/network/discovery.cpp`):**
        - Create a new method `void start()`. This will spawn a thread.
        - **Sender Thread:**
            - Create a `udp::socket` for broadcasting.
            - In a loop (e.g., every 5 seconds), send a discovery packet ("CHAT_DISCOVERY_PING") to the broadcast address (e.g., `255.255.255.255`) on a specific port (e.g., `9001`).
        - **Receiver Thread:**
            - Create a `udp::socket` to listen on port `9001`.
            - When it receives a "CHAT_DISCOVERY_PING", it should *not* respond to the broadcast address. Instead, it should send a "CHAT_DISCOVERY_PONG" message directly back to the sender's IP address. This message should include its own hostname.
    2.  **App Integration (`src/core/app.cpp`):**
        - The `App` will hold the `Discovery` service.
        - When the `Discovery` service receives a "PONG", it needs a way to tell the `App`. Add a callback to `Discovery` that `App` can provide.
        - When `App` is notified of a new peer, it should check if it already knows about that peer. If not, it adds it to the `peers_` vector. This will dynamically update the `PeerList` UI.

---