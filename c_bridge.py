"""Bridge module connecting Python Flask with the compiled C backend engine (bin/c_engine.exe)."""
import subprocess
import json
import os
import sys
from typing import Dict, Any, List, Optional

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
BIN_DIR = os.path.join(BASE_DIR, "bin")
C_ENGINE_EXE = os.path.join(BIN_DIR, "c_engine.exe" if sys.platform.startswith("win") else "c_engine")


def run_c_engine(command: str, *args: str) -> Dict[str, Any]:
    """Executes the compiled C engine with the specified subcommand and arguments,
    returning parsed JSON output.
    """
    if not os.path.exists(C_ENGINE_EXE):
        # Proactively compile if not present
        compile_c_engine()

    cmd = [C_ENGINE_EXE, command] + [str(a) for a in args]
    try:
        result = subprocess.run(
            cmd,
            cwd=BASE_DIR,
            capture_output=True,
            text=True,
            timeout=10,
            check=False
        )

        stdout = result.stdout.strip()
        if not stdout:
            if result.stderr:
                return {"status": "error", "message": result.stderr.strip()}
            return {"status": "empty"}

        try:
            return json.loads(stdout)
        except json.JSONDecodeError:
            # If stdout is raw text or multiline
            return {"status": "raw", "output": stdout, "stderr": result.stderr}

    except subprocess.TimeoutExpired:
        return {"status": "error", "message": "C engine execution timed out"}
    except Exception as e:
        return {"status": "error", "message": f"Execution error: {str(e)}"}


def compile_c_engine() -> bool:
    """Compiles the C source files into bin/c_engine.exe using GCC."""
    os.makedirs(BIN_DIR, exist_ok=True)
    c_files = [
        os.path.join(BASE_DIR, "c_src", f)
        for f in [
            "transaction.c",
            "fraud_rules.c",
            "linked_list.c",
            "graph.c",
            "queue.c",
            "stack.c",
            "bst.c",
            "hash_map.c",
            "algorithms.c",
            "c_engine.c"
        ]
    ]

    compile_cmd = ["gcc", "-Wall", "-Wextra", "-std=c99"] + c_files + ["-o", C_ENGINE_EXE, "-lm"]
    res = subprocess.run(compile_cmd, cwd=BASE_DIR, capture_output=True, text=True)
    return res.returncode == 0
