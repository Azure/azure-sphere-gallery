IP=$1

echo "Making requests to $IP:5000"

echo "CA and device certs"
curl --cacert Azsphere/certs/ca-bundle.pem --cert Azsphere/certs/device-cert.pem --key Azsphere/certs/device-key.pem https://$IP:5000/ -w "%{ssl_verify_result}"
echo ""
echo "Device certs only"
curl --cert Azsphere/certs/device-cert.pem --key Azsphere/certs/device-key.pem https://$IP:5000/
echo ""
echo "CA certs only"
curl --cacert Azsphere/certs/ca-bundle.pem https://$IP:5000/