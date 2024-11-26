import csv
import sys
import os
import numpy as np
import argparse
import matplotlib.pyplot as plt
from typing import List, Literal, Tuple, TypeVar, cast

import plotly.graph_objects as pgo

Values = List[int | float]
PhaseType = Literal["contraction"] | Literal["relaxation"]
Phase = Tuple[PhaseType, int, int | float, int, int]
"""Format: [phase_type ('contraction' or 'relaxation'), phase_number, phase_value, start_index, end_index]."""


def sanitize_path(path: str) -> str:
    return path.replace("\\", "/")


def read_vector_from_csv(file_path: str) -> Values:
    with open(file_path, mode="r") as file:
        reader = csv.reader(file)
        vector = []
        for row in reader:
            vector.extend([float(value) for value in row if value.strip() != ""])
        return vector


def to_float_or_str(value: str) -> float | str:
    try:
        return float(value)
    except ValueError:
        return value


T = TypeVar("T")


def read_vector_of_vectors_from_csv(file_path: str) -> List[T]:  # type: ignore
    with open(file_path, mode="r") as file:
        reader = csv.reader(file)
        vector_of_vectors = []
        for row in reader:
            vector_of_vectors.append([to_float_or_str(value) for value in row])
        return vector_of_vectors


def plot_video_events(
    values_path: str,
    events_path: str,
    video_name: str,
    save_path: str,
    fps: int,
):
    normalized_values = read_vector_from_csv(values_path)

    events: List[Values] = []
    if events_path != "":
        events = read_vector_of_vectors_from_csv(events_path)

    print("log: read_vector_from_csv")
    print("log: read_vector_of_vectors_from_csv")

    # Convert frame numbers to time in seconds
    time_values = [i / fps for i in range(len(normalized_values))]

    print("log: time_values")

    # Prepare figure
    fig = pgo.Figure()

    # Add line trace for normalized values
    fig.add_trace(
        pgo.Scatter(x=time_values, y=normalized_values, mode="lines", name="Values")
    )

    print("log: add traces")

    if events:
        # Normalize event values
        normalized_event_values = [event[1] for event in events]

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
                pgo.Scatter(
                    x=[event_time_mid],
                    y=[normalized_event_values[idx]],
                    mode="markers",
                    marker=dict(color="red"),
                    name=f"Event {ordinal_number}",
                )
            )

            print("log: add_trace")

    # Update layout
    fig.update_layout(
        title=f'Plot for {video_name}{f" - {len(events)} event{'' if len(events) == 1 else 's'}" if events is not None else ""}',
        xaxis_title="Time (seconds)",
        yaxis_title="XOR Values",
        showlegend=True,
    )

    print("log: update_layout")

    # Save plot as HTML file
    full_save_path = os.path.join(save_path, f"{video_name}_plot.html")
    fig.write_html(full_save_path)

    print("log: written to html")


def plot_contraction_relaxation_phases(
    normalized_values: Values,
    normalized_phases: List[Phase],
    video_name: str,
    save_path: str,
) -> None:
    """
    Plots cardiomyocyte activity with contraction and relaxation phases highlighted, ensuring paired numbering. Y values are always 0 by design.
    Args:
    normalized_values (list): List of normalized numeric values representing cardiomyocyte activity over time.
    normalized_phases (list): A list of normalied phases from locate_contractions_and_relaxations.
    """

    # Pair phases to share the same ordinal number
    paired_phases = []
    pair_number = 1  # Start numbering pairs
    for phase in normalized_phases:
        phase_type, _, phase_value, start_index, end_index, *_ = phase
        paired_phases.append(
            [phase_type, pair_number, phase_value, start_index, end_index]
        )

        if (
            phase_type == "relaxation"
        ):  # Increment pair number only after a relaxation phase
            pair_number += 1

    # Prepare figure
    fig = pgo.Figure()

    # Add paired phases (contractions and relaxations)
    for phase in paired_phases:
        phase_type, pair_number, _, start_index, end_index = phase
        phase_color = "blue" if phase_type == "contraction" else "green"
        phase_label = "Contraction" if phase_type == "contraction" else "Relaxation"

        # Highlight the phase as a shaded region
        fig.add_shape(
            type="rect",
            xref="x",
            yref="paper",
            x0=start_index,
            y0=0,
            x1=end_index,
            y1=1,
            line=dict(color=phase_color, width=0),
            fillcolor=phase_color,
            opacity=0.2,
            layer="below",
        )

        # Draw a semi-transparent, filled-in polygon for the phase
        opacity = 0.2
        fig.add_trace(
            pgo.Scatter(
                x=[start_index, end_index, end_index, start_index, start_index],
                y=[0, 0, 1, 1, 0],
                fill="toself",
                mode="lines",
                name=phase_label,
                text=phase_label,
                opacity=opacity,
                fillcolor=phase_color,
                marker=dict(color=phase_color, opacity=opacity),
                line=dict(color="rgba(0,0,0,0)"),  # disable shape stroke
            )
        )

    # Draw activity
    fig.add_trace(
        pgo.Scatter(
            x=list(range(len(normalized_values))),
            y=normalized_values,
            mode="lines",
            name="Activity",
            marker=dict(color="black"),
        )
    )

    # Update layout
    fig.update_layout(
        title=f"Contraction-relaxation analysis for {video_name}",
        xaxis_title="Frame",
        yaxis_title="XOR Values",
        showlegend=True,
    )

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

    parser_contraction_relaxation_analysis = subparsers.add_parser(
        "contraction_relaxation_analysis", help="Plot contraction-relaxation analysis."
    )
    parser_contraction_relaxation_analysis.add_argument(
        "normalized_values_path",
        type=str,
        help="Path to the CSV file containing the normalized values.",
    )
    parser_contraction_relaxation_analysis.add_argument(
        "normalized_phases_path",
        type=str,
        help="Path to the CSV file containing the normalized phases.",
    )
    parser_contraction_relaxation_analysis.add_argument(
        "video_name", type=str, help="Name of the video."
    )
    parser_contraction_relaxation_analysis.add_argument(
        "save_path", type=str, help="Path to save the plot."
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

                values_path = sanitize_path(options.values_path)
                events_path = sanitize_path(options.events_path)
                video_name = options.video_name
                save_path = sanitize_path(options.save_path)
                fps = options.fps

                plot_video_events(
                    values_path,
                    events_path,
                    video_name,
                    save_path,
                    fps,
                )

            case "contraction_relaxation_analysis":
                anyValidCommand = True

                normalized_values_path = sanitize_path(options.normalized_values_path)
                normalized_phases_path = sanitize_path(options.normalized_phases_path)
                video_name = options.video_name
                save_path = sanitize_path(options.save_path)

                plot_contraction_relaxation_phases(
                    normalized_values=read_vector_from_csv(normalized_values_path),
                    normalized_phases=cast(
                        List[Phase],
                        read_vector_of_vectors_from_csv(normalized_phases_path),
                    ),
                    video_name=video_name,
                    save_path=save_path,
                )

            case "auto_binarization_thresh":
                anyValidCommand = True

                xor_results_path = sanitize_path(options.xor_results_path)
                threshold_result_path = sanitize_path(options.threshold_result_path)
                save_path = sanitize_path(options.save_path)

                plot_auto_binarization_threshold(
                    xor_results_path, threshold_result_path, save_path
                )

    if not anyValidCommand:
        parser.print_help()
        sys.exit(1)
