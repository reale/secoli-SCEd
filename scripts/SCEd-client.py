#!/usr/bin/env python

import socket
from cmd import Cmd

MSGLEN = 2048

class MyPrompt(Cmd):
    def do_exit(self, inp):
        return True

    def do_connect(self, inp):
        self.sock = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        self.sock.connect(("localhost", 5005))

    def do_hello(self, inp):
        self.__mysend(b'HELLO')
        print(self.__myreceive())

    def __mysend(self, msg):
        totalsent = 0
        while totalsent < MSGLEN:
            sent = self.sock.send(msg[totalsent:])
            if sent == 0:
                return
            totalsent = totalsent + sent

    def __myreceive(self):
        chunks = []
        bytes_recd = 0
        while bytes_recd < MSGLEN:
            chunk = self.sock.recv(min(MSGLEN - bytes_recd, 2048))
            if chunk == '':
                raise RuntimeError("socket connection broken")
            chunks.append(chunk.decode("utf-8"))
            bytes_recd = bytes_recd + len(chunk)
            if chunk[-1:] == b'\n':
                break
        return ''.join(chunks)


MyPrompt().cmdloop()
