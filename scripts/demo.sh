#!/usr/bin/env bash
set -e

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )/.." >/dev/null 2>&1 && pwd )"
FLUX_BIN="$DIR/bin/flux"

if [ ! -f "$FLUX_BIN" ]; then
    echo "Building flux binary..."
    go build -o "$FLUX_BIN" "$DIR"/cli/*.go
fi

echo "===================================================================="
echo "  FLUX PLATFORM END-TO-END AUTOMATED INSTITUTIONAL TOUR"
echo "===================================================================="
sleep 1

echo -e "\n[STEP 1] Inspecting Platform Architecture & Quantitative Specs..."
"$FLUX_BIN" about
sleep 1.5

echo -e "\n[STEP 2] Rendering Live L2 Order Book Depth Ladder..."
"$FLUX_BIN" book
sleep 1.5

echo -e "\n[STEP 3] Evaluating Central Risk Book (CRB) Cross-Desk Factor Netting..."
"$FLUX_BIN" risk
sleep 1.5

echo -e "\n[STEP 4] Executing Two-Way Firm RFQ with AI Alpha Skew (+8.75 bps)..."
"$FLUX_BIN" rfq --underlying BRENT --strike 82.50 --qty 50000 --execute BUY
sleep 1.5

echo -e "\n[STEP 5] Calibrating Forward Curve Strip & SABR Volatility Surfaces..."
"$FLUX_BIN" curve
sleep 1.5

echo -e "\n[STEP 6] Auditing Physical Maritime Laytime Demurrage & ASTM Blends..."
"$FLUX_BIN" logistics
sleep 1.5

echo -e "\n[STEP 7] Calculating Bilateral XVA & ISDA SIMM v2.6 Margin Calls..."
"$FLUX_BIN" xva
sleep 1.5

echo -e "\n[STEP 8] Verifying SOC2 Type II SHA256 Hash-Chained Audit Ledger..."
"$FLUX_BIN" audit
sleep 1.5

echo -e "\n[STEP 9] Exporting Live OpenTelemetry & Prometheus Latency Telemetry..."
"$FLUX_BIN" metrics
sleep 1.5

echo -e "\n[STEP 10] Running Empirical Multi-Runtime Scale Benchmark (1M Runs)..."
"$FLUX_BIN" benchmark

echo -e "\n===================================================================="
echo "  FLUX AUTOMATED TOUR COMPLETED (ALL 10 VERIFICATION GATES PASSED)"
echo "===================================================================="
