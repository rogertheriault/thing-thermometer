// KEYS

// Keep this in config.h and keep it private!
// Even so, keep your devices secure, do not use this method
// for a production device

/*********WiFi settings*******/
#define WLAN_SSID "your-wifi-ssid"
#define WLAN_PASS "your-wifi-passphrase"
/*****************************/

/*********AWS IoT Thing settings*******/
#define AWS_REGION "us-east-1"
#define AWS_ENDPOINT "your-endpoint.iot.us-east-1.amazonaws.com"
#define THING_ID "your-thing-id"
//#define AWS_KEY_ID "your-aws-key-id"
//#define AWS_KEY_SECRET "your-aws-key-secret"

// put these in the data folder:
// You can get them from your Thing's security tab in the AWS IoT dashboard
// (attach a IAM policy to your thing as well)
// KEEP THESE PRIVATE!!!
// certificate.pem.crt
// private.pem.key
// aws-root-ca.pem

