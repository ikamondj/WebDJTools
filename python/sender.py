import re
import socket
import subprocess
import sounddevice as sd
import signal
import sys

# 1. Find a Tailscale IP (any 100.x.x.x)
def get_tailscale_ip():
    # Try socket.getaddrinfo first
    for iface in socket.getaddrinfo(socket.gethostname(), None):
        ip = iface[4][0]
        if re.match(r"^100\.\d+\.\d+\.\d+$", ip):
            return ip
    # Fallback: parse ipconfig output
    try:
        output = subprocess.check_output("ipconfig", shell=True, text=True)
        m = re.search(r"IPv4 Address.*?: (100\.\d+\.\d+\.\d+)", output)
        if m:
            return m.group(1)
    except subprocess.CalledProcessError:
        pass
    return None

# 2. Enumerate output-capable devices
def choose_output_device():
    devs = [d for d in sd.query_devices() if d['max_output_channels'] > 0]
    for idx, d in enumerate(devs):
        print(f"[{idx}] {d['name']}")
    choice = int(input("Select output device index: "))
    return devs[choice]['name']

def main():
    ts_ip = get_tailscale_ip()
    if not ts_ip:
        print("❌ Could not find a 100.* Tailscale IP.")
        sys.exit(1)
    print(f"→ Using Tailscale IP: {ts_ip}")

    dev_name = choose_output_device()
    print(f"→ Capturing from device: {dev_name}")

    # 3. Build GStreamer command
    gst_cmd = [
        r"C:\Program Files\gstreamer\1.0\msvc_x86_64\bin\gst-launch-1.0.exe",
        "-v",
        "wasapisrc", f"device=\"{dev_name}\"", "mode=loopback",
        "!", "audioconvert",
        "!", "audioresample",
        "!", "opusenc", "bitrate=128000",
        "!", "rtpopuspay",
        "!", "udpsink", f"host={ts_ip}", "port=5002", "sync=true", "async=false"
    ]

    # 4. Launch and manage subprocess
    proc = subprocess.Popen(gst_cmd)

    def cleanup(signum, frame):
        print("\n⏹ Shutting down sender pipeline…")
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        sys.exit(0)

    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    print("▶️ Sender pipeline running. Press Ctrl+C to stop.")
    ret = proc.wait()
    print(f"✅ Sender exited with code {ret}")

if __name__ == "__main__":
    main()
