import json
import paho.mqtt.client as mqtt
import argparse
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt

# ================= ARGPARSE =================
parser = argparse.ArgumentParser()
parser.add_argument('--fs', type=int, default=360)
parser.add_argument('--time_span', type=int, default=60)
parser.add_argument('--dst_path', type=str, required=True)
args = parser.parse_args()

# ================= MQTT =====================
MQTT_BROKER = "localhost"
MQTT_TOPIC = "ecg/data"

# ================= DATA =====================
hist_list = np.array([])
MAX_SHOW = 1000
plt.ion()
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 5))
line1, = ax1.plot([], [], color='blue', label="Lead 1")
line2, = ax2.plot([], [], color='red', label="Lead 2")

for ax, title in zip([ax1, ax2], ["ECG Lead 1", "ECG Lead 2"]):
    ax.set_xlim(0, 360)
    ax.set_ylim(0, 1)
    ax.set_xlabel(title)
    ax.set_ylabel("Amplitude")
    ax.grid(True)
    ax.legend()
def on_message(client, userdata, msg):
    global hist_list

    data = json.loads(msg.payload.decode("utf-8"))
    raw = data.get("input1", [])
    new_data = np.array(raw)
    hist_list = np.concatenate((hist_list, new_data))

    show_data = hist_list[-MAX_SHOW:]
    x = np.arange(len(show_data))

    line1.set_data(range(len(show_data)), show_data)

    ax1.set_xlim(0, len(show_data))
    fig.canvas.draw()
    fig.canvas.flush_events()


    if len(hist_list) >= args.fs * args.time_span:
        client.disconnect()

# ================= MQTT START =================
client = mqtt.Client()
client.on_message = on_message
client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC)

print(f"Listening MQTT {MQTT_BROKER} / {MQTT_TOPIC}")
client.loop_forever()

# ================= SAVE CSV =================
df = pd.DataFrame(hist_list, columns=["ECG"])
df.to_csv(args.dst_path, index=False)
print(f"Finish saving CSV to {args.dst_path}")
