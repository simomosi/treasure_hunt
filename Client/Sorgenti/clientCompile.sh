#!/bin/bash
gcc client.c client_interface.c -o client -Wall -pedantic &&
chmod 755 client &&
mv client ..
echo "Digita un tasto per continuare..."
read tasto
