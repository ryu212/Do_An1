import os
import pandas as pd
import numpy as np 
from scipy.signal import butter, filtfilt
import os
from scipy import signal


FOLDER_RAW = "../DATA_RAW"
def butter_lowpass(cutoff, fs, order=2):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='low')
    return b, a

def lowpass_filter(data, cutoff=40, fs=360, order=2):
    b, a = butter_lowpass(cutoff, fs, order)
    y = filtfilt(b, a, data)
    return y

def butter_highpass(cutoff, fs, order=2):
    nyq = 0.5 * fs
    normal_cutoff = cutoff / nyq
    b, a = butter(order, normal_cutoff, btype='high')
    return b, a

def highpass_filter(data, cutoff=0.5, fs=360, order=2):
    b, a = butter_highpass(cutoff, fs, order)
    y = filtfilt(b, a, data)
    return y

def notch_filter(data, f0 = 50, Q = 35, fs = 360):
    b, a = signal.iirnotch(f0, Q, fs)
    y = filtfilt(b, a, data)
    return y
def get_data(path: str):
    df = pd.read_csv(path)
    ECG = df["ECG"].to_numpy()
    return ECG 

def filter_folder(path):
    lst_dir = os.listdir(path)
    os.makedirs("lp", exist_ok= True)
    os.makedirs("lp_40", exist_ok=True)
    os.makedirs("hp", exist_ok= True)
    os.makedirs("lp_hp", exist_ok= True)
    os.makedirs("notch", exist_ok= True)
    os.makedirs("bandpass_notch", exist_ok= True)
    for file in lst_dir: 
            ECG = get_data(path+'/'+file)
            ECG_lp_150 = lowpass_filter(ECG, cutoff=150, fs=360, order=2)
            ECG_hp_0_5 = highpass_filter(ECG, cutoff=0.05, fs=360, order=2)
            ECG_lp_hp = lowpass_filter(ECG_hp_0_5, cutoff=150, fs=360, order=2)
            ECG_lp_40 = lowpass_filter(ECG, cutoff=40, fs=360, order=2)
            ECG_notch = notch_filter(ECG)
            ECG_bandpass_notch = notch_filter(ECG_lp_hp)
            df_lp = pd.DataFrame({"ECG": ECG_lp_150})
            df_hp = pd.DataFrame({"ECG": ECG_hp_0_5})
            df_lpxhp = pd.DataFrame({"ECG": ECG_lp_hp})
            df_notch = pd.DataFrame({"ECG": ECG_notch})
            df_bandpass_notch = pd.DataFrame({"ECG": ECG_bandpass_notch})
            df_lp_40 = pd.DataFrame({"ECG": ECG_lp_40})
            df_lp.to_csv("lp/lp_"+file, index = False)
            df_hp.to_csv("hp/hp_"+file, index = False)
            df_lpxhp.to_csv("lp_hp/lp_hp_"+file, index = False)
            df_notch.to_csv("notch/notch_"+file, index = False)
            df_bandpass_notch.to_csv("bandpass_notch/bandpass_notch_"+file, index = False)
            df_lp_40.to_csv("lp_40/lp_40_" + file, index = False)




if __name__ == "__main__":
    filter_folder(FOLDER_RAW)