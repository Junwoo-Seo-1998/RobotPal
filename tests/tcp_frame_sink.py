import argparse
import socket
import struct
import time


def recv_exact(conn: socket.socket, n: int) -> bytes | None:
    buf = bytearray()
    while len(buf) < n:
        try:
            chunk = conn.recv(n - len(buf))
        except (ConnectionResetError, TimeoutError, OSError):
            return None
        if not chunk:
            return None
        buf.extend(chunk)
    return bytes(buf)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=9998)
    parser.add_argument("--duration", type=float, default=20.0)
    args = parser.parse_args()

    start = time.perf_counter()
    frames = 0
    bytes_total = 0

    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
        server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        server.bind((args.host, args.port))
        server.listen(1)
        server.settimeout(max(1.0, args.duration))
        conn, addr = server.accept()
        with conn:
            conn.settimeout(1.0)
            while time.perf_counter() - start < args.duration:
                header = recv_exact(conn, 4)
                if header is None:
                    break
                (size,) = struct.unpack("<L", header)
                payload = recv_exact(conn, size)
                if payload is None:
                    break
                frames += 1
                bytes_total += size

    elapsed = max(1e-6, time.perf_counter() - start)
    print(
        f"[SINK] elapsed={elapsed:.2f}s frames={frames} "
        f"fps={frames/elapsed:.2f} mbps={(bytes_total*8)/(elapsed*1_000_000):.2f}"
    )


if __name__ == "__main__":
    main()
