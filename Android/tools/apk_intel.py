import zipfile
import sys
import os



def banner():
    print("""
\033[92m
 __   __   ___    ___ ____  
 \ \ / /  / _ \  |_ _|  _ \ 
  \ V /  | | | |  | || | | |
   \_/   | |_| |  | || |_| |
    |_|   \___/  |___|____/ 
\033[0m
\033[91m ╔══════════════════════════════════════╗
 ║   APK INTEL GATHERER  v1.0          ║
 ║   Android Security Research Tool    ║
 ║   Author  : V0ID                    ║
 ║   Mission : Getting cracked         ║
 ╚══════════════════════════════════════╝\033[0m

\033[93m [*] Initializing recon sequence....\033[0m
\033[96m [*] Target Platform : Android 15 (API 35)\033[0m
\033[90m ─────────────────────────────────────────\033[0m
    """)



# Magic byte signatures for common file types
MAGIC_BYTES = {
    b'\x1f\x8b': 'GZIP compressed',
    b'\x50\x4b': 'ZIP archive',
    b'\x7f\x45': 'ELF binary (Linux/Android native)',
    b'\x64\x65\x78': 'DEX bytecode',
    b'\x25\x50\x44\x46': 'PDF document',
    b'\x89\x50\x4e\x47': 'PNG image',
    b'\xff\xd8\xff': 'JPEG image',
    b'\x53\x51\x4c\x69': 'SQLite database',
}


def detect_magic(data):
    # Check the first 8 bytes against known signatures
    for magic, description in MAGIC_BYTES.items():
        if data.startswith(magic):
            return description

    return 'Unknown / raw data'


def analyze_apk(apk_path):
    # check first whether the file exists
    if not os.path.exists(apk_path):
        print(f"[-] File not found: {apk_path}")
        sys.exit(1)


    print(f"[+] Target : {apk_path}")
    print(f"[+] Size   : {os.path.getsize(apk_path) / 1024 / 1024:.2f} MB\n")


    # If APK is a zip - open directly without extracting
    with zipfile.ZipFile(apk_path, 'r') as apk:
        all_files = apk.namelist()


        # Count dex files
        dex_files = [ f for f in all_files if f.endswith('.dex')]
        print(f"[+] Dex files found     : {len(dex_files)}")
        for dex in dex_files:
            info = apk.getinfo(dex)
            print(f"    -> {dex} ({info.file_size / 1024:.1f} KB)")

        print()

        # Extract third party libraries from properties files if available
        props = [ f for f in all_files if f.endswith('.properties')]
        print(f"[+] Third party SDKs detected : {len(props)}")
        for prop in props:
            lib_name = os.path.basename(prop).replace('.properties', '')
            print(f"    -> {lib_name}")



        print()


        # Looking for some juicy stuffs
        interesting = [f for f in all_files if any(
            f.endswith(ext) for ext in
            ['.json', '.sql', '.xml', '.proto', '.db', '.key', '.pem=', '.cert']
            )]

        print(f"[+] Interesting files : {len(interesting)}")
        for f in interesting[:50]:
            print(f"    -> {f}")

        print()


        # Checking for native libraries - C(God choosen)/C++ code
        native_lib = [ f for f in all_files if f.endswith('.so')]
        if native_lib:
            print(f"[+] Native libraries (.so) : {len(native_lib)}")
            for lib in native_lib:
                print(f"    -> {lib}")

        else:
            print(f"[!] No native libraries found.")


        # scanning .bin files for hidden file types
        bin_files = [ f for f in all_files if f.endswith('.bin')]
        if bin_files:
            print(f"\n[+] Binary files - magic byte scan :")
            for binf in bin_files:
                data = apk.read(binf)
                magic = detect_magic(data)
                print(f"    -> {binf}")
                print(f"    Real type : {magic}")
                print(f"    Size      : {len(data) / 1024:.1f} KB")


if len(sys.argv) < 2:
    print(f"Usage: python3 {sys.argv[0]} <path_to_apk>")
    sys.exit(1)




banner()
analyze_apk(sys.argv[1])
