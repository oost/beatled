#!/bin/sh

set -e 

# Create CSR
openssl req -new -newkey rsa:4096 -nodes -keyout snakeoil.key -out snakeoil.csr             

# Create PEM cert
openssl x509 -req -sha256 -days 365 -in snakeoil.csr -signkey snakeoil.key -out snakeoil.pem

# Create DH Params

 openssl dhparam -out dhparam.pem 1024