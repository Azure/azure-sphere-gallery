# Make root CCA

IP=$1

echo "Making cert config for $IP"

sed "s/IP_ADDRESS/$IP/g" certs-config/server.conf.template > certs-config/server.conf

echo ""
echo "------------------------------------------------------------------------"
echo "Making root CA cert"
openssl req -new -nodes -x509 -days 365 -config certs-config/rootca.conf -out generated-certs/rootca-cert.pem -keyout generated-certs/rootca-key.pem -extensions v3_ca

# Intermediate cert
echo ""
echo "------------------------------------------------------------------------"
echo "Making intermediate cert request"
openssl req -new -nodes -keyout generated-certs/intermediate-key.pem \
            -out generated-certs/intermediate.csr -config certs-config/intermediate.conf \
            -extensions v3_intermediate_ca

echo ""
echo "------------------------------------------------------------------------"
echo "Signing intermediate cert"

openssl x509 -req -days 364 -in generated-certs/intermediate.csr \
             -CA generated-certs/rootca-cert.pem -CAkey generated-certs/rootca-key.pem \
             -CAcreateserial -out generated-certs/intermediate-cert.pem \
             -extensions v3_intermediate_ca -extfile certs-config/intermediate.conf

# Server cert

echo ""
echo "------------------------------------------------------------------------"
echo "Making server cert request"
openssl req -sha256 -nodes -newkey rsa:2058 -keyout generated-certs/server-key.pem -out generated-certs/server.csr -config certs-config/server.conf

echo ""
echo "------------------------------------------------------------------------"
echo "Signing server cert"
 openssl x509 -req -days 363 -in generated-certs/server.csr -CA generated-certs/intermediate-cert.pem -CAkey generated-certs/intermediate-key.pem -CAcreateserial \
              -out generated-certs/server-cert.pem -extensions req_extensions -extfile certs-config/server.conf

# Device cert

echo ""
echo "------------------------------------------------------------------------"
echo "Making device cert request"
openssl req -sha256 -nodes -newkey rsa:2058 -keyout generated-certs/device-key.pem -out generated-certs/device.csr -config certs-config/device.conf

echo ""
echo "------------------------------------------------------------------------"
echo "Signing device cert"
openssl x509 -req -days 363 -in generated-certs/device.csr -CA generated-certs/rootca-cert.pem -CAkey generated-certs/rootca-key.pem -CAcreateserial -out generated-certs/device-cert.pem

cat generated-certs/rootca-cert.pem generated-certs/intermediate-cert.pem > Azsphere/certs/ca-bundle.pem
cp generated-certs/device-cert.pem generated-certs/device-key.pem Azsphere/certs/
cp generated-certs/rootca-cert.pem generated-certs/server-cert.pem generated-certs/server-key.pem server-certs
