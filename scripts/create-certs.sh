#! /bin/bash

# https://devopscube.com/create-self-signed-certificates-openssl/

set -e 
# set -x

if [[ $# -lt 1 ]]
then
  echo "Error: No domain name argument provided"
  echo "Usage: $0 <domain> [certs_folder]"
  exit 1
fi

DOMAIN=$1
CERTS_FOLDER=${2:-"~/certs"}

echo "- Creating certs folder $CERTS_FOLDER 📂"
mkdir -p "$CERTS_FOLDER"

cd "$CERTS_FOLDER"

# Create root CA & Private key
echo "- Creating root certificate 🦷"

openssl req -x509 \
            -sha256 -days 356 \
            -nodes \
            -newkey rsa:4096 \
            -subj "/CN=${DOMAIN}/C=US/L=New York" \
            -keyout rootCA.key -out rootCA.crt

# Generate Private key

echo "- Generating private key 🔑"

openssl genrsa -out "${DOMAIN}.key" 4096

# Create csf conf

cat > csr.conf <<EOF
[ req ]
default_bits = 4096
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
CN = ${DOMAIN}

[ req_ext ]
subjectAltName = @alt_names

[ alt_names ]
DNS.1 = ${DOMAIN}
DNS.2 = www.${DOMAIN}
IP.1 = 127.0.0.1

EOF

# create CSR request using private key
echo "- Generating certificate signing request ✍️"

openssl req -new -key ${DOMAIN}.key -out ${DOMAIN}.csr -config csr.conf

# Create a external config file for the certificate

cat > cert.conf <<EOF

authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = ${DOMAIN}

EOF

# Create SSl with self signed CA

echo "- Generating SSL with self signed CA"

openssl x509 -req \
    -in ${DOMAIN}.csr \
    -CA rootCA.crt -CAkey rootCA.key \
    -CAcreateserial -out ${DOMAIN}.crt \
    -days 365 \
    -sha256 -extfile cert.conf

# Create CSR
echo "- Generating PEM key"

openssl req -new             \
            -newkey rsa:4096 \
            -nodes           \
            -keyout key.pem  \
            -out cert.csr    \
            -subj "/CN=${DOMAIN}/C=US/L=New York" 

        

# Create PEM cert
echo "- Generating PEM certificate"
openssl x509 -req             \
             -sha256          \
             -days 365        \
             -in cert.csr     \
             -signkey key.pem \
             -out cert.pem

# Create DH Params
echo "- Generating DH params"
openssl dhparam -out dh_param.pem 2048