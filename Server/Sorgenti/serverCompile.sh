#!/bin/bash
gcc -o server server.c manager.c interface.c winner.c -lpthread -Wall -pedantic -lcrypt &&
chmod 755 server &&
mv server ..
echo "Digita un tasto per continuare..."
read tasto
