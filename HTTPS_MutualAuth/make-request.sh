IP=$1

echo "Making requests to $IP:5000"

echo "CA and device certs"
curl --cacert Azsphere/certs/ca-bundle.pem -d "foo" --cert Azsphere/certs/device-cert.pem --key Azsphere/certs/device-key.pem https://$IP:5000/ -w "%{ssl_verify_result}"
echo ""
echo "Device certs only"
curl -d "foo" --cert Azsphere/certs/device-cert.pem --key Azsphere/certs/device-key.pem https://$IP:5000/
echo ""
echo "CA certs only"
curl --cacert Azsphere/certs/ca-bundle.pem -d "foo" https://$IP:5000/