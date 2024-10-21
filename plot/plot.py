import csv
import sys
import os
import plotly.graph_objects as go
import shutil

def remove_folder(folder_path):
    """Remove a folder and all its contents."""
    if os.path.isdir(folder_path):
        try:
            shutil.rmtree(folder_path)
            print(f"Successfully removed: {folder_path}")
        except Exception as e:
            print(f"Error: {e}")
    else:
        print(f"The path {folder_path} is not a directory.")

def normalize_values(values):
    if not values:
        return []
    min_value = min(values)
    max_value = max(values)

    normalized_values = [(x - min_value) / (max_value - min_value) for x in values]

    return normalized_values

def read_vector_from_csv(file_path):
    with open(file_path, mode='r') as file:
        reader = csv.reader(file)
        vector = []
        for row in reader:
            vector.extend([float(value) for value in row if value.strip() != ''])
        return vector
      
def read_vector_of_vectors_from_csv(file_path):
    with open(file_path, mode='r') as file:
        reader = csv.reader(file)
        vector_of_vectors = []
        for row in reader:
            vector_of_vectors.append([float(value) for value in row])
        return vector_of_vectors

def plot_events(valuesPath, eventsPath, video_name, save_path, fps, pathToRemove, normalize_flag):
    print("log: path to remove:" + pathToRemove)
    values = read_vector_from_csv(valuesPath)
    
    events = []
    if(eventsPath != ""): 
      events = read_vector_of_vectors_from_csv(eventsPath)
    
    print("log: read_vector_from_csv")
    print("log: read_vector_of_vectors_from_csv")
    
    normalized_values = []
    
    if(normalize_flag):
      normalized_values = normalize_values(values)
    else:
      normalized_values = values
    
    print("log: normalize_values")
        
    # Convert frame numbers to time in seconds
    time_values = [i / fps for i in range(len(values))]
    
    print("log: time_values")

    # Prepare figure
    fig = go.Figure()

    # Add line trace for normalized values
    fig.add_trace(go.Scatter(x=time_values, y=normalized_values, mode='lines', name='Values'))
    
    print("log: add_trace")

    if events:
        # Normalize event values
        event_values = [event[1] for event in events]
        
        normalized_event_values = []
        
        if(normalize_flag):
          normalized_event_values = normalize_values(event_values)
        else:
          normalized_event_values = event_values
        
        print("log: normalized_event_values")

        # Add red markers for events
        for idx, event in enumerate(events):
            ordinal_number, value1, time1, time2 = event
            event_time = (time1 + time2) / 2  # Use the midpoint of time1 and time2 as event time

            # Convert frame numbers to time in seconds
            event_time1 = time1 / fps
            event_time2 = time2 / fps
            event_time_mid = event_time / fps

            # Add shape for the event as a line segment on x-axis
            fig.add_shape(type="rect",
                          xref="x",
                          yref="paper",
                          x0=event_time1,
                          y0=0,
                          x1=event_time2,
                          y1=1,
                          line=dict(color="red", width=2),
                          fillcolor="red",
                          opacity=0.3,
                          layer="below",
                          name=f'Event {ordinal_number}')
            
            print("log: add_shape")

            # Add red marker for the normalized event value
            fig.add_trace(go.Scatter(x=[event_time_mid], y=[normalized_event_values[idx]], mode='markers', marker=dict(color='red'), name=f'Event {ordinal_number}'))
            
            print("log: add_trace")

            print("log: path from which we remove files: " + pathToRemove)
            
            remove_folder(pathToRemove)

    # Update layout
    if(normalize_flag):
      fig.update_layout(title=f'Plot for {video_name} - {len(events)} events',
                        xaxis_title='Time (seconds)',
                        yaxis_title='Normalized Values',
                        showlegend=True)
    else:
      fig.update_layout(title=f'Plot for {video_name}',
                        xaxis_title='Time (seconds)',
                        yaxis_title='XOR Values',
                        showlegend=True)
    
    print("log: update_layout")

    # Save plot as HTML file
    full_save_path = os.path.join(save_path, f'{video_name}_plot.html')
    fig.write_html(full_save_path)
    
    print("log: written to html")

if __name__ == "__main__":
    if len(sys.argv) < 6:
        print("Usage: plot.exe <valuesPath> <eventsPath> <video_name> <save_path> [fps] <pathToRemove> [normalize_flag]")
        sys.exit(1)
    print(f"Received arguments: {sys.argv}")

    valuesPath = sys.argv[1].replace("\\", "/")
    eventsPath = sys.argv[2].replace("\\", "/")
    video_name = sys.argv[3]
    save_path = sys.argv[4].replace("\\", "/")
    fps = int(sys.argv[5]) 
    pathToRemove = sys.argv[6].replace("\\", "/")
    normalize_flag = int(sys.argv[7]) # 0 / 1
    
    plot_events(valuesPath, eventsPath, video_name, save_path, fps, pathToRemove, normalize_flag)