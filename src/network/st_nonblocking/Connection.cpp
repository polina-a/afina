#include "Connection.h"

#include <sys/socket.h>
#include <sys/uio.h>

#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start() {
   std::cout << "Start" << std::endl;
   _event.data.fd = _socket;
   _event.data.ptr = this;
   _event.events = EPOLLIN;

 }

// See Connection.h
void Connection::OnError() {
   std::cout << "OnError" << std::endl;
   is_alive = false;
   _event.events = EPOLLERR;
 }


// See Connection.h
void Connection::OnClose() {
   std::cout << "OnClose" << std::endl;
   is_alive = false;
   _event.events = EPOLLHUP;}

// See Connection.h
void Connection::DoRead() {
   std::cout << "DoRead" << std::endl;

         try {
             int just_read = -1;
             while ((just_read = read(_socket, client_buffer, sizeof(client_buffer))) > 0) {
                read_bytes+=just_read;

           // Single block of data read from the socket could trigger inside actions a multiple times,
           // for example:
                 // - read#0: [<command1 start>]
                 // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
                 while (read_bytes > 0) {
                     // There is no command yet
                     if (!command_to_execute) {
                         std::size_t parsed = 0;
                         if (parser.Parse(client_buffer, read_bytes, parsed)) {
                             // There is no command to be launched, continue to parse input stream
                             // Here we are, current chunk finished some command, process it
                             command_to_execute = parser.Build(arg_remains);
                             if (arg_remains > 0) {
                                 arg_remains += 2;
                             }
                         }

                         // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                         // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                         if (parsed == 0) {
                             break;
                         } else {
                             std::memmove(client_buffer, client_buffer + parsed, read_bytes - parsed);
                             read_bytes -= parsed;
                         }
                     }

                     // There is command, but we still wait for argument to arrive...
                     if (command_to_execute && arg_remains > 0) {
                         // There is some parsed command, and now we are reading argument
                         std::size_t to_read = std::min(arg_remains, std::size_t(read_bytes));
                         argument_for_command.append(client_buffer, to_read);

                         std::memmove(client_buffer, client_buffer + to_read, read_bytes - to_read);
                         arg_remains -= to_read;
                         read_bytes -= to_read;
                     }

                     // Thre is command & argument - RUN!
                     if (command_to_execute && arg_remains == 0) {

                         std::string result;
                         command_to_execute->Execute(*_pStorage, argument_for_command, result);

                         // Send response
                         result += "\r\n";
                         if (outputs.empty()) {_event.events |= EPOLLOUT;}
                         outputs.push_back(result);
                         // Prepare for the next command
                         command_to_execute.reset();
                         argument_for_command.resize(0);
                         parser.Reset();
                     }
                 } // while (read_bytes)
                 if (read_bytes < 0) { OnError(); return; }
          }
        }catch (std::runtime_error &ex) {
              outputs.push_back("ERROR\r\n");
              _event.events |= EPOLLOUT;
          }
          if (!(_event.events & EPOLLOUT) && !(_event.events & EPOLLIN)) {OnClose();}
}

// See Connection.h
void Connection::DoWrite() {
   std::cout << "DoWrite" << std::endl;

    struct iovec iov[outputs.size()];

   for (size_t i = 0; i != outputs.size(); ++i) {
       iov[i].iov_base = &outputs[i][0];
       iov[i].iov_len = outputs[i].size();
   }
   written_bytes = writev(_socket, iov, outputs.size());
   if (written_bytes <= 0) {
       OnError();
       return;
   }
   auto last = outputs.begin();
   while (written_bytes > 0) {
       if (written_bytes >= last->size()) {
           written_bytes -= last->size();
           ++last;
       } else {
           last->erase(last->begin(), last->begin() + written_bytes);
           break;
       }
   }
   outputs.erase(outputs.begin(), last);
   if (!outputs.size()) {
       _event.events &= !EPOLLOUT;
   }
   if (!(_event.events & EPOLLOUT) && !(_event.events & EPOLLIN)) {
       OnClose();
   }
  }

} // namespace STnonblock
} // namespace Network
} // namespace Afina
