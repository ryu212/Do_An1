import numpy as np
from scipy import signal
import pywt
def filter_wavelet(signal_list,
                   walet='sym4', # walet test: sym4, sym8,db4, coif4, coif5
                   level=4, # mức phân rã wavelet, level > => tách được nhiều dải tần, nhưng level quá >, winsize < sẽ gây lỗi và mất chi tiết
                   win_size=360, # cửa sổ trượt, > sẽ giups ước lượng nhiễu tốt hơn -> chậm
                   # threshold type ap hệ số detail
                   #'soft': giảm gợn -> giảm nhẹ biên độ đỉnh
                   #'hard': giữ nguyên hệ số lớn, sắc nét nhưng dễ tạo artefact/gợn
                   mode='soft',
                   threshold_scale=1.0, # ngưỡng lọc nhiễu T = threshold * sigma * sqrt(2*ln(N))
                   blend='hann', # blend: "hann" nhân các windown làm mượt biên; 'none' hoặc 'rect' giữ nguyên
                   # chế độ sử lý biên khi phân rã/tái tạo
                   ## dwt_mode: 'zero', 'constant', 'symmetric', 'reflect', 'antisymmetric', 'antireflect', 'periodic', 'periodization', 'smooth'
                   # ảnh hưởng đến chất lượng đầu cuối
                   dwt_mode='symmetric'):
    ecg_signal=  signal_list
    # bước nhảy khi trượt
    jump_size = max(1, win_size // 2) # 50% chồng lấp
    def denoise_one_windown(seg):
      seg = np.array(seg)
      # Tự giảm level để phù hợp với win_size,
      windown = pywt.Wavelet(walet)
      max_level = pywt.dwt_max_level(data_len=len(seg), filter_len=windown.dec_len)
      use_level = min(level, max_level)
      if use_level == 0:
            return seg.copy()
      # phân rã wavelet: coeffs = [cA_L, cD_L, ..., cD_1]
      coeffs = pywt.wavedec(seg, walet, level=use_level, mode=dwt_mode)
      # sigma: ước lượng bằng đoạn nhiễu cao tần (emg/muscle noise, nhiễu đo/adc)
      # median(|N(0,1)|)=Φ−1(0.75)≈0.67448975
      # sigma = median(|d|) / 0.6745
      d_high = coeffs[-1]
      sigma = (np.median(np.abs(d_high)) / 0.6745) if len(d_high) > 0 else 0.0
      # universal threshold
      # threshold = threshold_scale * sigma * sqrt(2*ln(n_window))
      nwin = len(seg)
      threshold = threshold_scale * sigma * np.sqrt(2.0 * np.log(max(nwin, 2)))
      # giữ approximation (cA) (data tần thấp mang hình dạng của ecg), threshold toàn bộ detail (cD)
      new_coeffs = [coeffs[0]]
      for d in coeffs[1:]:
          new_coeffs.append(pywt.threshold(d, threshold, mode=mode))
      # tái tạo và cắt về đúng độ dài cửa sổ ban đầu
      y = pywt.waverec(new_coeffs, walet, mode=dwt_mode)
      return y[:len(seg)]

    len_signal = len(ecg_signal)
    if win_size >= len_signal:
      return denoise_one_windown(ecg_signal).tolist()
    # tạo trọng số  ghép các cửa sổ  để mượt biên
    window_weights = np.ones(win_size)
    if blend == 'hann':
      window_weights = np.hanning(win_size)
      if np.allclose(window_weights, 0):
        window_weights = np.ones(win_size)
    # overlap-add: cộng dồn các cửa sổ đã lọc và chuẩn hoá theo tổng trọng số
    out = np.zeros(len_signal, dtype=float)
    weight = np.zeros(len_signal, dtype=float)
    start = 0
    while start < len_signal :
      end = start + win_size
      if end <= len_signal:
        seg = ecg_signal[start:end]
        y_seg = denoise_one_windown(seg)
        out[start:end] += (y_seg * window_weights)
        weight[start:end] += window_weights
      else:
        seg = ecg_signal[start:len_signal]
        pad_len = win_size - len(seg)
        seg_pad = np.pad(seg, (0, pad_len), mode='edge')
        y_seg = denoise_one_windown(seg_pad)[:len(seg)]
        w_cut = window_weights[:len(seg)]
        out[start:len_signal] += y_seg * w_cut
        weight[start:len_signal] += w_cut
        break
      start += jump_size
    weight[weight == 0] = 1.0
    clean = out / weight
    return clean.tolist()


