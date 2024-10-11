import os
import plotly.graph_objects as go

def plot_events(values, events, video_name, save_path, fps=None):
    """
    Generates a line plot with optional red markers for specified events.

    Args:
        values (list): List of normalized numeric values to plot.
        events (list of list): List of events where each sublist contains [ordinal_number, value1, time1, time2].
                               If empty, no events will be marked.
        video_name (str): Name of the video to be used in the plot title.
        save_path (str): Path where the plot HTML file should be saved.
        fps (int, optional): Frames per second of the video, used to convert frame numbers to time. If None, frame numbers are used.

    Returns:
        None
    """
    # Determine x-axis values based on fps
    if fps is not None:
        # Convert frame numbers to time in seconds
        x_values = [i / fps for i in range(len(values))]
        x_axis_title = 'Time (seconds)'
    else:
        # Use frame numbers as x-axis values
        x_values = list(range(len(values)))
        x_axis_title = 'Frame number'

    # Prepare figure
    fig = go.Figure()

    # Add line trace for normalized values
    fig.add_trace(go.Scatter(x=x_values, y=values, mode='lines', name='Values'))

    # If events list is not empty, add event markers and shapes
    if events:
        for idx, event in enumerate(events):
            ordinal_number, value1, time1, time2 = event
            event_time = (time1 + time2) / 2  # Use the midpoint of time1 and time2 as event time

            if fps is not None:
                # Convert frame numbers to time in seconds
                event_time1 = time1 / fps
                event_time2 = time2 / fps
                event_time_mid = event_time / fps
            else:
                # Use frame numbers directly
                event_time1 = time1
                event_time2 = time2
                event_time_mid = event_time

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

            # Add red marker for the event value
            fig.add_trace(go.Scatter(x=[event_time_mid], y=[value1], mode='markers', marker=dict(color='red'), name=f'Event {ordinal_number}'))

    # Update layout
    fig.update_layout(title=f'Plot for {video_name} - {len(events)} events',
                      xaxis_title=x_axis_title,
                      yaxis_title='Normalized Values',
                      showlegend=True)

    # Save plot as HTML file
    full_save_path = os.path.join(save_path, f'{video_name}_plot.html')
    fig.write_html(full_save_path)