"""
Clean Akutan data (via normalization, interpolation, and convolution filtering.)
"""

import pandas as pd
import numpy as np
from sklearn.preprocessing import MinMaxScaler
import os


data = pd.read_csv("data/raw/akutan_raw.csv")
data['time'] = pd.to_datetime(data['time'])
data = data.sort_values('time').reset_index(drop=True)


seconds = (data['time'] - data['time'].iloc[0]).dt.total_seconds()
data['time_norm'] = seconds / seconds.max()


numeric_cols = data.select_dtypes(include=[np.number]).columns.drop(['rid', 'time_norm'], errors='ignore')


data[numeric_cols] = data[numeric_cols].interpolate(method='linear')

#convolution filter
window_size = 20

window = np.hanning(window_size)
window = window / window.sum() 


for col in numeric_cols:
    
    data[col] = np.convolve(data[col], window, mode='same')


scaler = MinMaxScaler()
data[numeric_cols] = scaler.fit_transform(data[numeric_cols])


final_data = pd.concat([data[['time', 'time_norm']], data[numeric_cols]], axis=1)
final_data.to_csv("data/cleaned/akutan_cleaned.csv", index=False)

print("Data saved to 'data/cleaned/akutan_cleaned.csv'")