import json
import matplotlib 
matplotlib.use('TkAgg')
import matplotlib.pyplot as plt
import paho.mqtt.client as mqtt
import os
from scipy.signal import butter, filtfilt
import numpy as np
from cnn_lstm import CNN_LSTM
import torch
import pywt
from scipy.signal import find_peaks
 


MQTT_BROKER = "localhost"
MQTT_TOPIC = "ecg/data"
WINDOW_SIZE = 360
label2id = {'N': 0, 'S': 1, 'V': 2, 'F': 3, 'Q': 4}
id2label = {0: 'Normal beats', 1: 'Supraventricular ectopic beats', 2: 'Ventricular ectopic beats', 3: 'Fusion beats', 4: 'Unknown / Paced beats '}
model = CNN_LSTM()
state_dict = torch.load("cnn_lstm_ecg.pth", map_location="cpu")
model.load_state_dict(state_dict)
model.eval()

def butter_bandpass(lowcut, highcut, fs, order=2):
    nyq = 0.5 * fs   
    low = lowcut / nyq
    high = highcut / nyq
    b, a = butter(order, [low, high], btype='band')
    return b, a

def bandpass_filter(data, lowcut=0.5, highcut=40, fs=360, order=2):
    b, a = butter_bandpass(lowcut, highcut, fs, order=order)
    y = filtfilt(b, a, data) 
    return y
def find_r_peaks(ecg, fs=360):
    peaks, properties = find_peaks(
        ecg,
        distance=int(0.25 * fs),      # 250ms
        height=0.7              
    )
    return peaks

def infer_ecg(signal_1d: np.ndarray):
    """
    signal_1d: (360,)
    """
    signal_1d = (signal_1d - np.mean(signal_1d)) / (np.std(signal_1d) + 1e-6)
    x = torch.tensor(signal_1d, dtype=torch.float32)
    x = x.unsqueeze(0).unsqueeze(1)  # (1, 1, 360)
    with torch.no_grad():
        logits = model(x)
        prob = torch.softmax(logits, dim=1)
        pred = torch.argmax(prob, dim=1)
    return pred.item(), prob.squeeze().numpy()
def WaveletTransform(signal,wave_func, ):
    sampling_rate = 360 
    t = np.linspace(0, 10, 10 * sampling_rate)

    scales = np.arange(1,50)
    coefficients, frequencies = pywt.cwt(signal, scales, wave_func, sampling_period=1/sampling_rate)

    selected_coefficients = np.sum(coefficients[5:30, :], axis=0)

    selected_coefficients /= np.max(np.abs(selected_coefficients))
    return selected_coefficients

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

lead1_data = np.array([])
lead2_data = np.array([])

def on_message(client, userdata, msg):
    global lead1_data, lead2_data
    lead1_inferrence = None
    payload = msg.payload.decode('utf-8')
    data = json.loads(payload)
    new_lead1 = np.array(data.get("input1", []))
    new_lead2 = np.array(data.get("input2", []))
    #new_lead1 = bandpass_filter(data = new_lead1)
    lead1_data= np.concatenate((lead1_data, new_lead1))
    lead2_data= np.concatenate((lead2_data, new_lead2))
    
    # 1000 sample
    lead1_data = lead1_data[-1000:]
    lead2_data = lead2_data[-1000:]
    
    label_text = ""
    if len(lead1_data) >= WINDOW_SIZE:
        window = lead1_data[-WINDOW_SIZE:]
        #window = (window - window.mean()) / (window.std() + 1e-6)
        pred, prob = infer_ecg(window)
       
        label_text = f"{id2label[pred]} ({prob[pred]:.2f})"
        print(f"[ECG] Predicted class: {id2label[pred]}, confidence: {prob[pred]:.3f}")

    data_inference = None
    if len(lead1_data) > 720:
        lead1_inferrence = lead1_data[-720:]
    
    if lead1_inferrence is not None:
        peaks = find_r_peaks(lead1_inferrence, 360)
        for peak in peaks:
            left = peak - 180
            right = peak + 180
            if left < 0 or right > len(lead1_inferrence):
                continue

            data_inference = lead1_inferrence[left:right]
            # data_inference -= data_inference.mean()
            # data_inference = (data_inference - data_inference.mean()) / \
            #                 (data_inference.std() + 1e-6)
            pred, prob = infer_ecg(data_inference)
    line1.set_data(range(len(lead1_data)), lead1_data)

    ax1.set_xlim(0, len(lead1_data))
    ax2.set_xlim(0, len(lead2_data))
    ax1.set_title(f"Lead 1 - Predicted: {label_text}", fontsize=12, color='green')
    if data_inference is not None:
        print("Mean X = ", data_inference.mean())
        print("Max X = ", data_inference.max())
    fig.canvas.draw()
    fig.canvas.flush_events()


client = mqtt.Client()
client.on_message = on_message

client.connect(MQTT_BROKER, 1883, 60)
client.subscribe(MQTT_TOPIC)

print(f"Đang nghe MQTT tại broker: {MQTT_BROKER}, topic: {MQTT_TOPIC}")
client.loop_forever()
