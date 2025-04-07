import json
import plotly.graph_objects as go
import os
from collections import defaultdict
import numpy as np
import hashlib

def process_json_line(line):
    try:
        content = json.loads(line)
        
        if 'data' in content:
            data = content['data']
            return data
        
        if 'CudaEvent' in content:
            event = content['CudaEvent']
            kernel = event.get('kernel', {})
            short_name = kernel.get('shortName')
            start_mks = float(event.get('startNs', 0)) / 1000.0
            end_mks = float(event.get('endNs', 0)) / 1000.0
            return short_name, start_mks, end_mks
        if 'NvtxEvent' in content:
            event = content['NvtxEvent']
            short_name = '[NVTX]' + event.get('Text')
            start_mks = float(event.get('Timestamp', 0)) / 1000.0
            end_mks = float(event.get('EndTimestamp', 0)) / 1000.0
            return short_name, start_mks, end_mks
    except json.JSONDecodeError:
        print("Error decoding JSON.")
        return None, None, None

def moving_average(data, window_size):
    """Apply moving average to smooth data."""
    return np.convolve(data, np.ones(window_size)/window_size, mode='valid')

def get_color_from_name(name):
    """Generate a consistent color for a given kernel name."""
    hash_object = hashlib.md5(name.encode())
    hex_color = hash_object.hexdigest()[:6]
    return f'#{hex_color}'

def plot_top_kernel_durations(events, filename, top_n=7, window_size=5):
    kernel_durations = defaultdict(list)
    
    for short_name, start_mks, end_mks in events:
        if short_name:
            duration = end_mks - start_mks
            kernel_durations[short_name].append(duration)
    
    avg_durations = {kernel: sum(durations)/len(durations) for kernel, durations in kernel_durations.items()}
    
    top_kernels = sorted(avg_durations.items(), key=lambda item: item[1], reverse=True)[:top_n]
    
    fig = go.Figure()
    for kernel, _ in top_kernels:
        durations = kernel_durations[kernel]
        smoothed_durations = moving_average(durations, window_size)
        color = get_color_from_name(kernel)  # Use consistent color for each kernel
        
        fig.add_trace(go.Scatter(
            x=list(range(len(smoothed_durations))),
            y=smoothed_durations,
            mode='lines',
            name=f'{kernel}, avg: {avg_durations[kernel]:.2f} \u03BCs',
            line=dict(color=color)
        ))
    
    fig.update_layout(
        title=f'Top {top_n} Consuming Kernels, {filename}',
        xaxis_title='Event Index',
        yaxis_title='Duration, \u03BCs',
        legend_title='Routine'
    )
    
    fig.show()

def main(filename):
    strings = {}
    events = []
    exclusion_list = ["RunKernel"]
    
    with open(filename, 'r') as file:
        for line in file:
            data = process_json_line(line)
            if data:
                if isinstance(data, list):
                    for i, name in enumerate(data):
                        strings[i] = name
                        # print(f"{i} {name}")
                elif isinstance(data, tuple):
                    short_name, start_mks, end_mks = data
                    if (short_name and short_name.isdigit()):
                        short_name = strings.get(int(short_name), short_name)
                    if short_name in exclusion_list:
                        continue
                    events.append((short_name, start_mks, end_mks))
    
    plot_top_kernel_durations(events, os.path.basename(filename), window_size=10)

# Replace 'your_file.json' with your actual JSON file path
if __name__ == "__main__":
    main('C:/Users/si/Documents/NVIDIA Nsight Systems/Projects/Project 2/DeconvConv.json')
    