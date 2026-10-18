import argparse
import pathlib
import sys
import time


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--duration", type=float, default=22.0)
    args = parser.parse_args()

    repo_root = pathlib.Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(repo_root / "RobotPal-python" / "src"))

    from robotpal._core.server import SimulatorServer

    SimulatorServer.instance()
    time.sleep(args.duration)


if __name__ == "__main__":
    main()
