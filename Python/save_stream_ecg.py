import json
import paho.mqtt.client as mqtt
import argparse
import numpy as np
import pandas as pd

parser = argparse.ArgumentParser(description="Tool to save ECG stream to CSV")
parser.add_argument('--fs', type=int, default=360, help="sample rate (sps)")
parser.add_argument('--time_span', type=int, default=60, help="time span the save will last in second (s)")
parser.add_argument('--dst_path', type=str, required=True, help="path to save the csv file")
args = parser.parse_args()

MQTT_BROKER = "localhost"
MQTT_TOPIC = "ecg/data"

hist_list = []

def on_message(client, user_data, message):
    global hist_list
    data = json.loads(message.payload.decode("utf-8"))
    new_lead1 = data.get("input1", [])
    if not isinstance(new_lead1, list):
        return

    hist_list.extend(new_lead1)

    if len(hist_list) >= args.fs * args.time_span:
        client.disconnect()

if __name__ == '__main__':
    client = mqtt.Client()
    client.on_message = on_message

    client.connect(MQTT_BROKER, 1883, 60)
    client.subscribe(MQTT_TOPIC)

    print(f"Listening MQTT {MQTT_BROKER} / {MQTT_TOPIC}")
    client.loop_forever()
    
    df = pd.DataFrame(np.array(hist_list), columns=['ECG'])
    df.to_csv(args.dst_path, index=False)
    print("Finish saving")
