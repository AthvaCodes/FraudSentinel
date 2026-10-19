@echo off
echo ===================================================
echo Compiling FraudSentinel Native C Engine...
echo ===================================================

if not exist bin mkdir bin

gcc -Wall -Wextra -std=c99 c_src/transaction.c c_src/fraud_rules.c c_src/linked_list.c c_src/graph.c c_src/queue.c c_src/stack.c c_src/bst.c c_src/hash_map.c c_src/algorithms.c c_src/c_engine.c -o bin/c_engine.exe -lm
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation of bin/c_engine.exe failed.
    exit /b %ERRORLEVEL%
)

gcc -Wall -Wextra -std=c99 c_src/transaction.c c_src/fraud_rules.c c_src/linked_list.c c_src/graph.c c_src/queue.c c_src/stack.c c_src/bst.c c_src/hash_map.c c_src/algorithms.c c_src/c_engine.c -o bin/bank_cli.exe -lm
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Compilation of bin/bank_cli.exe failed.
    exit /b %ERRORLEVEL%
)

echo [SUCCESS] C Binaries built successfully in bin/
echo Run web app with: python app.py
echo Run CLI with: .\bin\bank_cli.exe
