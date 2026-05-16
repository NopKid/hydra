import base64
import sys

def decode(encoded):
    padding = (4 - len(encoded) % 4) % 4
    encoded += "=" * padding

    try:
        decoded = base64.urlsafe_b64decode(encoded)
        print(f"[+] Encoded : {encoded}")
        print(f"[+] Decoded (raw) : {decoded}")

        try:
            print(f"[+] Decoded (text) : {decoded.decode('utf-8')}")
        except:
            print(f"[!] Not readable text - raw bytes only")

    except Exception as e:
        print(f"[-] Failed  : {e}")

if len(sys.argv) < 2:
    print(f"Usage: python3 {sys.argv[0]} <base64_string>")
    sys.exit(1)

decode(sys.argv[1])
