#
# Copyright (c) 2026 Vladislav Soulgard (vsoulgard at gmail dot com)
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#

import pandas as pd
import os
import cpuinfo
import plotly.graph_objects as go
from subprocess import check_output

lib_boost_name = "Boost.Prometheus"
lib_prom_cpp_name = "prometheus-cpp"

colors = {
    lib_prom_cpp_name: '#ef1b04',
    lib_boost_name: '#0a6cdd'
}

def run_benchmark(bench_path, executable_name):
    """Runs benchmark and returns (DataFrame, metadata lines)"""
    exe_path = os.path.join(bench_path, executable_name)
    out = check_output([exe_path]).decode()

    meta = []
    data = []
    for line in out.split('\n'):
        if not line:
            continue
        if line.startswith('#'):
            meta.append(line[1:].strip())
        else:
            data.append(line)
    from io import StringIO
    return pd.read_csv(StringIO('\n'.join(data))), meta


def load_pair(run_benchmark, bench_path, boost_name, prom_name):
    boost, boost_meta = run_benchmark(bench_path, boost_name)
    boost["library"] = lib_boost_name
    prom, prom_meta = run_benchmark(bench_path, prom_name)
    prom["library"] = lib_prom_cpp_name
    df = pd.concat([boost, prom], ignore_index=True)
    return df, boost_meta or prom_meta


def environment_lines(meta):
    cpu = cpuinfo.get_cpu_info().get("brand_raw", "unknown CPU")
    meta.insert(0, f"CPU: {cpu}")
    return meta

def add_metadata_footer(fig, meta):
    fig.add_annotation(
        xref="paper", yref="paper",
        x=0, y=0,
        xanchor="left", yanchor="top",
        align="left",
        text="<br>".join(environment_lines(meta)),
        showarrow=False,
        font=dict(size=11, color="#7a7a7a"),
    )

def thread_stats(df, num_labels=None):
    d = df if num_labels is None else df[df["labels"] == num_labels]
    threads = sorted(d["threads"].unique())

    prom_ops, boost_ops, differences = [], [], []
    for t in threads:
        prom = d[(d["library"] == lib_prom_cpp_name) & (d["threads"] == t)]["ops_per_sec"].values[0]
        boost = d[(d["library"] == lib_boost_name) & (d["threads"] == t)]["ops_per_sec"].values[0]
        prom_ops.append(prom / 1e6)
        boost_ops.append(boost / 1e6)
        differences.append(boost / prom)

    return threads, prom_ops, boost_ops, differences


def make_table(x_values, left_vals, right_vals, differences,
    font_size=12, row_height=26,
    values_fmt="{:.2f}", header_suffix="",
    x_header="Threads"):
    n = len(x_values)
    zebra = ["#ffffff", "#f4f4f4"] * ((n + 1) // 2)

    return go.Table(
        header=dict(
            values=[x_header,
                    f"prometheus-cpp{header_suffix}",
                    f"Boost.Prometheus{header_suffix}",
                    "Difference"],
            fill_color="#3d3d3d",
            font=dict(color="white", size=font_size),
            align="center",
            line_color="white",
        ),
        cells=dict(
            values=[
                x_values,
                [values_fmt.format(v) for v in left_vals],
                [values_fmt.format(v) for v in right_vals],
                [f"{v:.2f}x" for v in differences],
            ],
            fill_color=[zebra[:n], zebra[:n], zebra[:n], ["#dceffc"] * n],
            font=dict(
                size=font_size - 1,
                color=[["#333333"] * n, ["#333333"] * n,
                       ["#333333"] * n, ["#0a6cdd"] * n],
            ),
            align="center",
            line_color="white",
            height=row_height,
        ),
    )


def add_line_traces(fig, df, row, col, showlegend=True):
    for lib in (lib_prom_cpp_name, lib_boost_name):
        d = df[df["library"] == lib].sort_values("threads")
        fig.add_trace(
            go.Scatter(
                x=d["threads"],
                y=d["ops_per_sec"] / 1e6,
                mode="lines+markers",
                name=lib,
                line=dict(color=colors[lib], width=2),
                marker=dict(size=8),
                showlegend=showlegend,
                hovertemplate=(
                    "<b>%{fullData.name}</b><br>"
                    "Threads: %{x}<br>"
                    "M ops/s: %{y:.2f}<extra></extra>"
                ),
            ),
            row=row, col=col,
        )

def metric_stats(df):
    counts = sorted(df["metric_count"].unique())
    prom_ops, boost_ops, differences = [], [], []
    for c in counts:
        prom = df[(df["library"] == lib_prom_cpp_name) & (df["metric_count"] == c)]["ops_per_sec"].values[0]
        boost = df[(df["library"] == lib_boost_name) & (df["metric_count"] == c)]["ops_per_sec"].values[0]
        prom_ops.append(prom / 1e3)
        boost_ops.append(boost / 1e3)
        differences.append(boost / prom)
    return counts, prom_ops, boost_ops, differences

def add_line_traces_metric(fig, df, row, col, showlegend=True):
    for lib in (lib_prom_cpp_name, lib_boost_name):
        d = df[df["library"] == lib].sort_values("metric_count")
        fig.add_trace(
            go.Scatter(
                x=d["metric_count"],
                y=d["ops_per_sec"] / 1e3,
                mode="lines+markers",
                name=lib,
                line=dict(color=colors[lib], width=2),
                marker=dict(size=8),
                showlegend=showlegend,
                hovertemplate=(
                    "<b>%{fullData.name}</b><br>"
                    "Metrics: %{x}<br>"
                    "K ops/s: %{y:.2f}<extra></extra>"
                ),
            ),
            row=row, col=col,
        )
