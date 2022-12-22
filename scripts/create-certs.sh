#!/bin/bash

set -e 

mkdir ~/certs 

cd ~/certs 

# Create CSR
openssl req -new -newkey rsa:4096 -nodes -keyout key.pem -out cert.csr             

# Create PEM cert
openssl x509 -req -sha256 -days 365 -in cert.csr -signkey key.pem -out cert.pem

# Create DH Params
openssl dhparam -out dh_param.pem 2048