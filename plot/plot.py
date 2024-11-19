import csv
import sys
import os
import plotly.graph_objects as go
import shutil
import argparse
import matplotlib.pyplot as plt
from typing import List

Values = List[int | float]


def remove_folder(folder_path: str):
    """Remove a folder and all its contents."""
    if os.path.isdir(folder_path):
        try:
            shutil.rmtree(folder_path)
            print(f"Successfully removed: {folder_path}")
        except Exception as e:
            print(f"Error: {e}")
    else:
        print(f"The path {folder_path} is not a directory.")


def normalize_values(values: Values) -> Values:
    if not values:
        return []
    min_value = min(values)
    max_value = max(values)

    normalized_values = [(x - min_value) / (max_value - min_value) for x in values]

    return normalized_values


def read_vector_from_csv(file_path: str) -> Values:
    with open(file_path, mode="r") as file:
        reader = csv.reader(file)
        vector = []
        for row in reader:
            vector.extend([float(value) for value in row if value.strip() != ""])
        return vector


def read_vector_of_vectors_from_csv(file_path: str) -> List[Values]:
    with open(file_path, mode="r") as file:
        reader = csv.reader(file)
        vector_of_vectors = []
        for row in reader:
            vector_of_vectors.append([float(value) for value in row])
        return vector_of_vectors


def plot_video_events(
    values_path: str,
    events_path: str,
    video_name: str,
    save_path: str,
    fps: int,
    path_to_remove: str,
    normalize_flag: bool,
):
    print("log: path to remove:" + path_to_remove)
    values = read_vector_from_csv(values_path)

    events: List[Values] = []
    if events_path != "":
        events = read_vector_of_vectors_from_csv(events_path)

    print("log: read_vector_from_csv")
    print("log: read_vector_of_vectors_from_csv")

    normalized_values: Values = normalize_values(values) if normalize_flag else values

    print("log: normalize_values")

    # Convert frame numbers to time in seconds
    time_values = [i / fps for i in range(len(values))]

    print("log: time_values")

    # Prepare figure
    fig = go.Figure()

    # Add line trace for normalized values
    fig.add_trace(
        go.Scatter(x=time_values, y=normalized_values, mode="lines", name="Values")
    )

    print("log: add_trace")

    if events:
        # Normalize event values
        event_values = [event[1] for event in events]

        normalized_event_values = []

        if normalize_flag:
            normalized_event_values = normalize_values(event_values)
        else:
            normalized_event_values = event_values

        print("log: normalized_event_values")

        # Add red markers for events
        for idx, event in enumerate(events):
            ordinal_number, _value1, time1, time2 = event
            event_time = (
                time1 + time2
            ) / 2  # Use the midpoint of time1 and time2 as event time

            # Convert frame numbers to time in seconds
            event_time1 = time1 / fps
            event_time2 = time2 / fps
            event_time_mid = event_time / fps

            # Add shape for the event as a line segment on x-axis
            fig.add_shape(
                type="rect",
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
                name=f"Event {ordinal_number}",
            )

            print("log: add_shape")

            # Add red marker for the normalized event value
            fig.add_trace(
                go.Scatter(
                    x=[event_time_mid],
                    y=[normalized_event_values[idx]],
                    mode="markers",
                    marker=dict(color="red"),
                    name=f"Event {ordinal_number}",
                )
            )

            print("log: add_trace")

            print("log: path from which we remove files: " + path_to_remove)

            remove_folder(path_to_remove)

    # Update layout
    fig.update_layout(
        title=f'Plot for {video_name}{f" - {len(events)} event{'' if len(events) == 1 else 's'}" if events is not None else ""}',
        xaxis_title="Time (seconds)",
        yaxis_title="Normalized Values" if normalize_flag else "XOR Values",
        showlegend=True,
    )

    print("log: update_layout")

    # Save plot as HTML file
    full_save_path = os.path.join(save_path, f"{video_name}_plot.html")
    fig.write_html(full_save_path)

    print("log: written to html")


def plot_auto_binarization_threshold(
    xor_results_path: str, threshold_result_path: str, save_path: str
):
    xor_results = read_vector_from_csv(xor_results_path)

    with open(threshold_result_path) as threshold_result_file:
        threshold = threshold_result_file.read().strip()

    plt.figure()
    plt.title("Automatic Binarization Threshold - XOR Results")
    plt.xlabel("Threshold")
    plt.ylabel("XOR Result")

    plt.plot(range(256), xor_results, label="XOR Results")
    plt.axvline(
        x=int(threshold), color="r", linestyle="--", label=f"Threshold: {threshold}"
    )
    plt.legend()
    plt.grid()

    out_file = os.path.join(save_path, "auto_binarization_threshold_plot.png")

    plt.savefig(out_file)

    print(f"log: written to {out_file}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot utility.")
    subparsers = parser.add_subparsers(help="plot <command>", dest="subparser_name")

    parser_video_events = subparsers.add_parser(
        "video_events", help="Plot video events."
    )
    parser_video_events.add_argument(
        "values_path", type=str, help="Path to the CSV file containing the values."
    )
    parser_video_events.add_argument(
        "events_path", type=str, help="Path to the CSV file containing the events."
    )
    parser_video_events.add_argument("video_name", type=str, help="Name of the video.")
    parser_video_events.add_argument(
        "save_path", type=str, help="Path to save the plot."
    )
    parser_video_events.add_argument(
        "fps", type=int, help="Frames per second of the video."
    )
    parser_video_events.add_argument("path_to_remove", type=str, help="Path to remove.")
    parser_video_events.add_argument(
        "normalize_flag",
        type=bool,
        help="Normalize the values.",
        default=False,
        nargs="?",
    )

    parser_auto_binarization_threshold = subparsers.add_parser(
        "auto_binarization_thresh",
        help="Plot automatic binarization threshold results.",
    )
    parser_auto_binarization_threshold.add_argument(
        "xor_results_path",
        type=str,
        help="Path to the CSV file containing the XOR results for each threshold.",
    )
    parser_auto_binarization_threshold.add_argument(
        "threshold_result_path",
        type=str,
        help="Path to the TXT file containing the calculated threshold.",
    )
    parser_auto_binarization_threshold.add_argument(
        "save_path", type=str, help="Path to save the plot."
    )

    argv = sys.argv[1:]

    anyValidCommand = False

    while argv:
        options, argv = parser.parse_known_args(argv)

        match options.subparser_name:
            case "video_events":
                anyValidCommand = True

                values_path = options.values_path.replace("\\", "/")
                events_path = options.events_path.replace("\\", "/")
                video_name = options.video_name
                save_path = options.save_path.replace("\\", "/")
                fps = options.fps
                path_to_remove = options.path_to_remove.replace("\\", "/")
                normalize_flag = options.normalize_flag

                plot_video_events(
                    values_path,
                    events_path,
                    video_name,
                    save_path,
                    fps,
                    path_to_remove,
                    normalize_flag,
                )

            case "auto_binarization_thresh":
                anyValidCommand = True

                xor_results_path = options.xor_results_path.replace("\\", "/")
                threshold_result_path = options.threshold_result_path.replace("\\", "/")
                save_path = options.save_path.replace("\\", "/")

                plot_auto_binarization_threshold(
                    xor_results_path, threshold_result_path, save_path
                )

    if not anyValidCommand:
        parser.print_help()
        sys.exit(1)