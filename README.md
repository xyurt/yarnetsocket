# YarnetSocket

**YarnetSocket** is a lightweight cross-platform socket abstraction layer for **Windows** and **Unix-like systems** (Linux, BSD, macOS).  
It provides a unified API for working with UDP sockets, addresses, and buffers without exposing platform-specific details.

## Features

- ✅ Cross-platform (WinSock2 on Windows, BSD sockets on Unix/macOS)  
- ✅ Simple address & buffer handling  
- ✅ Unified socket API (`create`, `bind`, `send`, `recv`, `set_option`, `get_option`)  
- ✅ Support for scatter/gather I/O (`sendmsg`, `recvmsg` on Unix, `WSASendTo`/`WSARecvFrom` on Windows)  
- ✅ Non-blocking mode support  
- ✅ Clean initialization/cleanup (`yarnet_socket_initialize`, `yarnet_socket_deinitialize`)

## Notes
You have to link "ws2_32.lib" on windows
