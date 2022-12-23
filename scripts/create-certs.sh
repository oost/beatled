#!/bin/bash

set -e 

mkdir ~/certs 

cd ~/certs 

# https://devopscube.com/create-self-signed-certificates-openssl/

# Create RootCA
openssl req -x509 -sha256 -days 356 -nodes -newkey rsa:2048 -subj "/CN=raspberrypi1.local/C=US/L=New York" \
            -keyout rootCA.key -out rootCA.crt 

# Create server key
openssl genrsa -out server.key 2048


cat > csr.conf <<EOF
[ req ]
default_bits = 2048
prompt = no
default_md = sha256
req_extensions = req_ext
distinguished_name = dn

[ dn ]
C = US
ST = New York
L = New York City
O = Beatled
OU = Beatled
CN = demo.mlopshub.com

[ req_ext ]
subjectAltName = @alt_names

[ alt_names ]
DNS.1 = raspberrypi1.local
# DNS.2 = www.demo.mlopshub.com
# IP.1 = 192.168.1.5
# IP.2 = 192.168.1.6

EOF

# 3. Generate Certificate Signing Request (CSR) Using Server Private Key
openssl req -new -key server.key -out server.csr -config csr.conf

# 4. Create a external file
cat > cert.conf <<EOF

authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = raspberrypi1.local

EOF

# 5. Generate SSL certificate With self signed CA

openssl x509 -req \
    -in server.csr \
    -CA rootCA.crt -CAkey rootCA.key \
    -CAcreateserial -out server.crt \
    -days 365 \
    -sha256 -extfile cert.conf

# Create CSR
openssl req -new -newkey rsa:4096 -nodes -keyout key.pem -out cert.csr             

# Create PEM cert
openssl x509 -req -sha256 -days 365 -in cert.csr -signkey key.pem -out cert.pem

# Create DH Params
openssl dhparam -out dh_param.pem 2048