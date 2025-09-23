import re
import subprocess
import sounddevice as sd
import signal
import sys

# 1. Pick a loopback/virtual device or default
def pick_output_device():
    devs = [d for d in sd.query_devices() if d['max_output_channels'] > 0]
    preferred = [
        d for d in devs
        if re.search(r"(loopback|virtual)", d['name'], re.IGNORECASE)
    ]
    if preferred:
        print(f"→ Using loopback device: {preferred[0]['name']}")
        return preferred[0]['name']
    # else default
    default_idx = sd.default.device[1]  # (input, output)
    default_dev = sd.query_devices()[default_idx]
    print(f"→ Using default output device: {default_dev['name']}")
    return default_dev['name']

def main():
    dev_name = pick_output_device()

    # 2. Build GStreamer command
    gst_cmd = [
        r"C:\Program Files\gstreamer\1.0\msvc_x86_64\bin\gst-launch-1.0.exe",
        "-v",
        "udpsrc", "port=5002",
            "caps=application/x-rtp,media=audio,encoding-name=OPUS,payload=96",
        "!", "rtpopusdepay",
        "!", "opusdec",
        "!", "audioconvert",
        "!", "audioresample",
        "!", "wasapisink", f"device=\"{dev_name}\"", "sync=true"
    ]

    # 3. Launch and manage subprocess
    proc = subprocess.Popen(gst_cmd)

    def cleanup(signum, frame):
        print("\n⏹ Shutting down receiver pipeline…")
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
        sys.exit(0)

    signal.signal(signal.SIGINT, cleanup)
    signal.signal(signal.SIGTERM, cleanup)

    print("▶️ Receiver pipeline running. Press Ctrl+C to stop.")
    ret = proc.wait()
    print(f"✅ Receiver exited with code {ret}")

if __name__ == "__main__":
    main()
