import json
import matplotlib 
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import paho.mqtt.client as mqtt
import os
from scipy.signal import butter, filtfilt
import numpy as np
# MQTT cấu hình
MQTT_BROKER = "localhost"
MQTT_TOPIC = "ecg/data"
def butter_bandpass(lowcut, highcut, fs, order=2):
    nyq = 0.5 * fs   # Tần số Nyquist
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def bandpass_filter(data, lowcut=0.5, highcut=60, fs=250, order=2):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = filtfilt(b, a, data)   # lọc hai chiều -> không trễ pha
    return y
# Bật chế độ interactive

plt.ion()

# Tạo figure và 2 subplot
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 5))
line1, = ax1.plot([], [], color='blue', label="Lead 1")
line2, = ax2.plot([], [], color='red', label="Lead 2")
# Callback khi có dữ liệu về
for ax, title in zip([ax1, ax2], ["ECG Lead 1", "ECG Lead 2"]):
    ax.set_xlim(0, 250)
    ax.set_ylim(-1, 1)
    ax.set_xlabel(title)
    ax.set_ylabel("Amplitude (V)")
    ax.grid(True)
    ax.legend()

# Buffer dữ liệu để hiển thị
lead1_data = np.array([])
lead2_data = np.array([])

def on_message(client, userdata, msg):
    global lead1_data, lead2_data

    payload = msg.payload.decode('utf-8')
    data = json.loads(payload)
    new_lead1 = np.array(data.get("input1", []))
    new_lead2 = np.array(data.get("input2", []))
    new_lead1 = bandpass_filter(data = new_lead1)
    # Thêm dữ liệu mới
    lead1_data= np.concatenate((lead1_data, new_lead1))
    lead2_data= np.concatenate((lead2_data, new_lead2))
    
    # Giữ lại tối đa 250 mẫu để hiển thị (rolling window)
    lead1_data = lead1_data[-1000:]
    lead2_data = lead2_data[-1000:]
    
    # Cập nhật dữ liệu cho line
    line1.set_data(range(len(lead1_data)), lead1_data)
    # line2.set_data(range(len(lead2_data)), lead2_data)

    # Cập nhật giới hạn X nếu cần
    ax1.set_xlim(0, len(lead1_data))
    ax2.set_xlim(0, len(lead2_data))

    # Redraw
    fig.canvas.draw()
    fig.canvas.flush_events()


# MQTT client setup
client = mqtt.Client()
client.on_message = on_message

client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC)

print(f"📡 Đang nghe MQTT tại broker: {MQTT_BROKER}, topic: {MQTT_TOPIC}")
client.loop_forever()
