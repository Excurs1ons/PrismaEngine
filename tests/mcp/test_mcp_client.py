#!/usr/bin/env python3
"""Prisma MCP Stdio Integration Test.

Tests a real MCPServer with TransportStdio by starting it as a subprocess,
sending JSON-RPC messages via stdin, and validating responses from stdout.

Usage:
    python tests/mcp/test_mcp_client.py
"""

import subprocess
import json
import sys
import os
import re
from pathlib import Path


def find_server():
    """Find the test_mcp_stdio server executable."""
    candidates = [
        Path(__file__).parent / ".." / ".." / "build" / "engine-windows-x64-debug" / "bin" / "Debug" / "test_mcp_stdio.exe",
        Path(__file__).parent / ".." / ".." / "build" / "tests" / "test_mcp_stdio.exe",
    ]
    for p in candidates:
        if p.exists():
            return p.resolve()
    return None


class MCPClient:
    """Minimal MCP client over stdio transport."""

    def __init__(self, process):
        self.proc = process
        self.req_id = 0

    def send_request(self, method: str, params: dict = None):
        """Send a JSON-RPC request and return parsed response."""
        self.req_id += 1
        msg = {
            "jsonrpc": "2.0",
            "id": self.req_id,
            "method": method,
            "params": params or {},
        }
        line = json.dumps(msg) + "\n"
        self.proc.stdin.write(line.encode("utf-8"))
        self.proc.stdin.flush()
        return self._read_response()

    def _read_response(self) -> dict:
        """Read next valid JSON-RPC response from stdout."""
        while True:
            line = self.proc.stdout.readline()
            if not line:
                raise RuntimeError("Server closed connection")
            text = line.decode("utf-8", errors="replace").strip()
            if not text:
                continue
            try:
                obj = json.loads(text)
                if isinstance(obj, dict) and "id" in obj:
                    return obj
                # Log messages (not JSON-RPC) are silently skipped
            except json.JSONDecodeError:
                continue

    def close(self):
        try:
            self.proc.stdin.close()
        except:
            pass
        try:
            self.proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            self.proc.terminate()
            self.proc.wait(timeout=2)


def test_initialize(client: MCPClient):
    """Test: initialize handshake."""
    print("[TEST] initialize...", end=" ", flush=True)
    resp = client.send_request("initialize", {
        "clientInfo": {"name": "test-client", "version": "1.0"},
        "maxTokensPerResponse": 2000,
    })
    assert resp.get("id") == 1, f"Expected id=1, got {resp.get('id')}"
    assert "result" in resp, f"No result in response: {resp}"
    assert resp["result"]["protocolVersion"] == "2025-03-26", \
        f"Unexpected protocol: {resp['result']['protocolVersion']}"
    assert "prisma_extensions" in resp["result"]["capabilities"], \
        "No prisma_extensions in capabilities"
    print("PASS")


def test_tools_list(client: MCPClient):
    """Test: tools/list."""
    print("[TEST] tools/list...", end=" ", flush=True)
    resp = client.send_request("tools/list")
    assert resp.get("id") == 2
    tools = resp["result"]["tools"]
    assert len(tools) == 2, f"Expected 2 tools, got {len(tools)}"
    names = {t["name"] for t in tools}
    assert "echo" in names, f"echo tool missing: {names}"
    assert "add" in names, f"add tool missing: {names}"
    print("PASS")


def test_tool_call_echo(client: MCPClient):
    """Test: tools/call with echo tool."""
    print("[TEST] tools/call echo...", end=" ", flush=True)
    resp = client.send_request("tools/call", {
        "name": "echo",
        "arguments": {"message": "hello mcp"},
    })
    assert resp.get("id") == 3
    assert resp["result"]["echo"] == "hello mcp", \
        f"Expected 'hello mcp', got {resp['result'].get('echo')}"
    print("PASS")


def test_tool_call_add(client: MCPClient):
    """Test: tools/call with add tool."""
    print("[TEST] tools/call add...", end=" ", flush=True)
    resp = client.send_request("tools/call", {
        "name": "add",
        "arguments": {"a": 40, "b": 2},
    })
    assert resp.get("id") == 4
    assert resp["result"]["result"] == 42, \
        f"Expected 42, got {resp['result'].get('result')}"
    print("PASS")


def test_unknown_method(client: MCPClient):
    """Test: unknown method returns error."""
    print("[TEST] unknown method...", end=" ", flush=True)
    resp = client.send_request("bogus_method")
    assert resp.get("id") == 5
    assert "error" in resp, f"Expected error, got {resp}"
    assert resp["error"]["code"] == -32601  # MethodNotFound
    print("PASS")


def test_notification(client: MCPClient):
    """Test: notification (no response expected)."""
    print("[TEST] notification (no response)...", end=" ", flush=True)
    # Send notification (no 'id' field)
    msg = json.dumps({
        "jsonrpc": "2.0",
        "method": "notifications/initialized",
        "params": {},
    }) + "\n"
    client.proc.stdin.write(msg.encode("utf-8"))
    client.proc.stdin.flush()
    # Notification should NOT produce a response.
    # Verify: the next response should still be for our next request.
    resp = client.send_request("tools/list")
    assert resp.get("id") is not None
    # If we got here without receiving an error, notification was handled correctly
    print("PASS")


def main():
    server_path = find_server()
    if not server_path:
        print("ERROR: test_mcp_stdio.exe not found.")
        print("Build it first with:")
        print("  cmake --build --preset engine-windows-x64-debug --target test_mcp_stdio")
        sys.exit(1)

    print(f"Starting server: {server_path}")
    proc = subprocess.Popen(
        [str(server_path)],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )

    client = MCPClient(proc)
    passed = 0
    failed = 0
    tests = [
        test_initialize,
        test_tools_list,
        test_tool_call_echo,
        test_tool_call_add,
        test_unknown_method,
        test_notification,
    ]

    for test in tests:
        try:
            test(client)
            passed += 1
        except Exception as e:
            print(f"FAIL: {e}")
            failed += 1

    client.close()

    # Print stderr from server
    stderr = proc.stderr.read().decode("utf-8", errors="replace")
    if stderr.strip():
        print(f"\n[Server stderr]\n{stderr.strip()}")

    print(f"\n{'='*40}")
    print(f"Results: {passed} passed, {failed} failed, {len(tests)} total")
    if failed > 0:
        sys.exit(1)
    print("ALL INTEGRATION TESTS PASSED")


if __name__ == "__main__":
    main()
