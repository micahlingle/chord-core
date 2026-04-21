#!/usr/bin/env python3

import argparse
import csv
import pathlib
import re
import subprocess
import sys
from dataclasses import dataclass
from typing import Iterable, List, Optional


SAMPLE_RATE_RE = re.compile(r"^Sample rate:\s+(?P<sample_rate>\d+)\s+Hz$")
BLOCK_RE = re.compile(
    r'^Block\s+\d+\s+start=(?P<start_seconds>[0-9.]+)s\s+samples=(?P<samples>\d+)\s+'
    r'chord="(?P<chord>[^"]+)"\s+confidence=(?P<confidence>[0-9.]+)$'
)


@dataclass
class BlockPrediction:
    start_seconds: float
    end_seconds: float
    chord: str


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Run chord_core_demo on a WAV file and convert the block output into "
            "a starter start_seconds,end_seconds,chord CSV."
        )
    )
    parser.add_argument("wav_path", help="Path to the WAV file to analyze.")
    parser.add_argument(
        "--demo",
        default="build/chord_core_demo",
        help="Path to the chord_core_demo executable. Default: build/chord_core_demo",
    )
    parser.add_argument(
        "--output",
        help=(
            "Output CSV path. Default: <wav stem>.labels.csv next to the WAV file."
        ),
    )
    parser.add_argument(
        "--include-unknown",
        action="store_true",
        help="Include Unknown chord spans in the output CSV.",
    )
    return parser.parse_args()


def run_demo(demo_path: pathlib.Path, wav_path: pathlib.Path) -> str:
    completed = subprocess.run(
        [str(demo_path), str(wav_path)],
        check=True,
        capture_output=True,
        text=True,
    )
    return completed.stdout


def parse_predictions(demo_output: str) -> List[BlockPrediction]:
    sample_rate: Optional[int] = None
    predictions: List[BlockPrediction] = []

    for line in demo_output.splitlines():
        sample_rate_match = SAMPLE_RATE_RE.match(line)
        if sample_rate_match:
            sample_rate = int(sample_rate_match.group("sample_rate"))
            continue

        block_match = BLOCK_RE.match(line)
        if not block_match:
            continue

        if sample_rate is None:
            raise ValueError("Could not parse sample rate before block output.")

        start_seconds = float(block_match.group("start_seconds"))
        samples = int(block_match.group("samples"))
        duration_seconds = float(samples) / float(sample_rate)
        end_seconds = start_seconds + duration_seconds

        predictions.append(
            BlockPrediction(
                start_seconds=start_seconds,
                end_seconds=end_seconds,
                chord=block_match.group("chord"),
            )
        )

    if not predictions:
        raise ValueError("No block predictions were parsed from chord_core_demo output.")

    return predictions


def merge_predictions(
    predictions: Iterable[BlockPrediction],
    include_unknown: bool,
) -> List[BlockPrediction]:
    merged: List[BlockPrediction] = []

    for prediction in predictions:
        if prediction.chord == "Unknown" and not include_unknown:
            continue

        if merged and merged[-1].chord == prediction.chord:
            merged[-1].end_seconds = prediction.end_seconds
            continue

        merged.append(
            BlockPrediction(
                start_seconds=prediction.start_seconds,
                end_seconds=prediction.end_seconds,
                chord=prediction.chord,
            )
        )

    return merged


def write_csv(output_path: pathlib.Path, predictions: Iterable[BlockPrediction]) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as output_file:
        writer = csv.writer(output_file)
        writer.writerow(["start_seconds", "end_seconds", "chord"])
        for prediction in predictions:
            writer.writerow(
                [
                    f"{prediction.start_seconds:.3f}",
                    f"{prediction.end_seconds:.3f}",
                    prediction.chord,
                ]
            )


def main() -> int:
    args = parse_args()
    wav_path = pathlib.Path(args.wav_path)
    demo_path = pathlib.Path(args.demo)

    if not wav_path.is_file():
        print(f"WAV file not found: {wav_path}", file=sys.stderr)
        return 1

    if not demo_path.is_file():
        print(f"Demo executable not found: {demo_path}", file=sys.stderr)
        return 1

    output_path = (
        pathlib.Path(args.output)
        if args.output
        else wav_path.with_suffix(".labels.csv")
    )

    demo_output = run_demo(demo_path, wav_path)
    merged_predictions = merge_predictions(
        parse_predictions(demo_output),
        include_unknown=args.include_unknown,
    )
    write_csv(output_path, merged_predictions)

    print(f"Wrote {len(merged_predictions)} label rows to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
