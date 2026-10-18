#!/usr/bin/env python3
import argparse
import importlib.util
import time
from pathlib import Path

server_path = Path(__file__).resolve().parents[2] / "RobotPal-python" / "src" / "robotpal" / "_core" / "server.py"
spec = importlib.util.spec_from_file_location("robotpal_benchmark_server", server_path)
server_module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(server_module)

parser = argparse.ArgumentParser()
parser.add_argument("--duration-seconds", type=float, required=True)
args = parser.parse_args()
server_module.SimulatorServer.instance()
time.sleep(args.duration_seconds)
