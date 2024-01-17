# !/bin/sh
# provided from: https://nextbigthings.info/secured-tls-connection-using-boost-asio-and-openssl-for-windows/
# info provided for rootca.crt and user.crt has to be different

openssl genrsa -out rootca.key 2048
openssl req -x509 -new -nodes -key rootca.key -days 36500 -out rootca.crt
openssl genrsa -out user.key 2048
openssl req -new -key user.key -out user.csr
openssl x509 -req -in user.csr -CA rootca.crt -CAkey rootca.key -CAcreateserial -out user.crt -days 36500

openssl verify -CAfile rootca.crt rootca.crt
openssl verify -CAfile rootca.crt user.crt
openssl verify -CAfile user.crt user.crt

openssl dhparam -out dh2048.pem 2048

