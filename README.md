This apparently uses LLDB to hook IOConnectCallMethod by overwriting
the function entry with a jump to the local re-implementation.
