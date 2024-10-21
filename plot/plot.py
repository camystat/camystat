import csv
import sys
import os
import plotly.graph_objects as go
import shutil
import argparse

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

def plot_events(valuesPath, eventsPath, video_name, save_path, fps, path_to_remove, normalize_flag):
    print("log: path to remove:" + path_to_remove)
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

            print("log: path from which we remove files: " + path_to_remove)
            
            remove_folder(path_to_remove)

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
    parser = argparse.ArgumentParser(description='Plot video events.')
    parser.add_argument('valuesPath', type=str, help='Path to the CSV file containing the values.')
    parser.add_argument('eventsPath', type=str, help='Path to the CSV file containing the events.')
    parser.add_argument('video_name', type=str, help='Name of the video.')
    parser.add_argument('save_path', type=str, help='Path to save the plot.')
    parser.add_argument('fps', type=int, help='Frames per second of the video.')
    parser.add_argument('path_to_remove', type=str, help='Path to remove.')
    parser.add_argument('normalize_flag', type=int, help='Normalize the values.', default=False, nargs='?')
    args = parser.parse_args()

    valuesPath = args.valuesPath.replace("\\", "/")
    eventsPath = args.eventsPath.replace("\\", "/")
    video_name = args.video_name
    save_path = args.save_path.replace("\\", "/")
    fps = args.fps
    path_to_remove = args.path_to_remove.replace("\\", "/")
    normalize_flag = args.normalize_flag

    
    plot_events(valuesPath, eventsPath, video_name, save_path, fps, path_to_remove, normalize_flag)