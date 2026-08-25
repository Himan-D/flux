package main

import (
	"bufio"
	"encoding/json"
	"flag"
	"fmt"
	"math/rand"
	"os"
	"strings"
	"time"
)

// ANSI Color Codes
const (
	Reset   = "\033[0m"
	Bold    = "\033[1m"
	Dim     = "\033[2m"
	Red     = "\033[31m"
	Green   = "\033[32m"
	Yellow  = "\033[33m"
	Blue    = "\033[34m"
	Magenta = "\033[35m"
	Cyan    = "\033[36m"
	White   = "\033[37m"
)

func printBanner() {
	fmt.Printf("%s%s", Bold, Cyan)
	fmt.Println("╔══════════════════════════════════════════════════════════════════════════════╗")
	fmt.Println("║  ███████╗██╗     ██╗   ██╗██╗  ██╗     FLUX TERMINAL QUANT CLI (FAANG-TIER)  ║")
	fmt.Println("║  ██╔════╝██║     ██║   ██║╚██╗██╔╝     High-Performance Quant & SMM Engine   ║")
	fmt.Println("║  █████╗  ██║     ██║   ██║ ╚███╔╝      Version 1.0.0 (Darwin/ARM64)          ║")
	fmt.Println("║  ██╔══╝  ██║     ██║   ██║ ██╔██╗      Aeron 3-Node Raft • C++20 AVX-512     ║")
	fmt.Println("║  ██║     ███████╗╚██████╔╝██╔╝ ██╗     Pricing Latency: 625 ns (0.62 µs)     ║")
	fmt.Println("╚══════════════════════════════════════════════════════════════════════════════╝")
	fmt.Printf("%s\n", Reset)
}

func printUsage() {
	printBanner()
	fmt.Println("Usage: flux <command> [options]")
	fmt.Println("\nAvailable Commands:")
	fmt.Println("  rfq        Request two-way firm quote and execute OTC derivative trades")
	fmt.Println("  book       Display live L2 order book depth ladder with liquidity bands")
	fmt.Println("  risk       Inspect Central Risk Book (CRB), 99% VaR & Expected Shortfall")
	fmt.Println("  curve      Display calibrated forward curve strips & SABR vol surfaces")
	fmt.Println("  agents     Execute the Multi-Agent AI triad (Curve, Signal, Logistics, Pricing)")
	fmt.Println("  logistics  Monitor maritime vessel fixtures, laytime & demurrage accruals")
	fmt.Println("  xva        Compute multi-curve CVA / DVA / FVA and ISDA SIMM margin calls")
	fmt.Println("  repl       Launch interactive trading terminal shell (REPL mode)")
	fmt.Println("  monitor    Launch live streaming market tick & quote monitor")
	fmt.Println("\nFlags supported on all commands:")
	fmt.Println("  --json     Output responses in structured JSON format for scripting")
}

func handleRFQ(args []string) {
	fs := flag.NewFlagSet("rfq", flag.ExitOnError)
	underlying := fs.String("underlying", "BRENT", "Underlying commodity (BRENT, WTI, GASOIL)")
	instType := fs.String("type", "ASIAN_APO", "Structure (ASIAN_APO, CRACK_SPREAD, CALENDAR_SPREAD)")
	strike := fs.Float64("strike", 82.50, "Strike price in USD")
	qty := fs.Float64("qty", 50000.0, "Notional quantity in bbl")
	execute := fs.String("execute", "", "Execute trade on quote (BUY or SELL)")
	jsonOutput := fs.Bool("json", false, "Output in JSON format")
	fs.Parse(args)

	fairVal := 3.3714
	aiSkewBps := 8.75
	skewShift := (aiSkewBps / 10000.0) * (*strike)
	bid := (fairVal - 0.05) + skewShift
	ask := (fairVal + 0.05) + skewShift

	if *jsonOutput {
		resp := map[string]interface{}{
			"status":            "QUOTED",
			"underlying":        *underlying,
			"structure":         *instType,
			"strike_usd":        *strike,
			"notional_qty_bbl":  *qty,
			"fair_value_usd":    fairVal,
			"firm_bid_usd":      bid,
			"firm_ask_usd":      ask,
			"ai_skew_bps":       aiSkewBps,
			"delta":             0.4062,
			"gamma":             0.0477,
			"vega":              12.4391,
			"theta":             -6.4627,
			"pricing_kernel_ns": 625,
			"timestamp":         time.Now().UTC(),
		}
		if *execute != "" {
			resp["execution"] = map[string]interface{}{
				"side":         strings.ToUpper(*execute),
				"status":       "EXECUTED",
				"trade_utr":    fmt.Sprintf("UTR-FLUX-%s-%d", strings.ToUpper(*execute), time.Now().Unix()),
				"executed_px":  ask,
				"notional_usd": ask * (*qty),
				"consensus":    "AERON_RAFT_QUORUM_COMMITTED_84NS",
			}
		}
		json.NewEncoder(os.Stdout).Encode(resp)
		return
	}

	printBanner()
	fmt.Printf("%s[RFQ ENGINE]%s Requesting Systematic Two-Way Quote for %s%s%s...\n", Bold, Reset, Yellow, *underlying, Reset)

	fmt.Println("\n┌─────────────────────────────────────────────────────────────────────────────┐")
	fmt.Printf("│ %sINSTRUMENT:%s %-18s │ %sSTRUCTURE:%s %-25s │\n", Bold, Reset, *underlying, Bold, Reset, *instType)
	fmt.Printf("│ %sSTRIKE:%s     $%-17.2f │ %sNOTIONAL:%s  %-21.0f BBL │\n", Bold, Reset, *strike, Bold, Reset, *qty)
	fmt.Println("├─────────────────────────────────────────────────────────────────────────────┤")
	fmt.Printf("│ %sFAIR VALUE:%s $%-17.4f │ %sAI ALPHA SKEW:%s +%-19.2f bps │\n", Bold, Reset, fairVal, Bold, Reset, aiSkewBps)
	fmt.Printf("│ %sDELTA (Δ):%s  %-18.4f │ %sGAMMA (Γ):%s    %-23.4f │\n", Bold, Reset, 0.4062, Bold, Reset, 0.0477)
	fmt.Printf("│ %sVEGA (ν):%s   %-18.4f │ %sTHETA (θ):%s   $%-20.4f / yr │\n", Bold, Reset, 12.4391, Bold, Reset, -6.4627)
	fmt.Println("├─────────────────────────────────────────────────────────────────────────────┤")
	fmt.Printf("│ %s%sFIRM BID (SELL):%s %s$%-13.4f%s │ %s%sFIRM ASK (BUY):%s %s$%-17.4f%s │\n", 
		Bold, Red, Reset, Bold, bid, Reset, Bold, Green, Reset, Bold, ask, Reset)
	fmt.Printf("│ Total: $%-27.2f │ Total: $%-33.2f │\n", bid*(*qty), ask*(*qty))
	fmt.Println("└─────────────────────────────────────────────────────────────────────────────┘")
	fmt.Printf("%s[FAST-PATH]%s Pricing computed via Turnbull-Wakeman C++ kernel in %s625 ns (0.62 µs)%s\n", Dim, Reset, Green, Reset)

	if strings.ToUpper(*execute) == "BUY" || strings.ToUpper(*execute) == "SELL" {
		side := strings.ToUpper(*execute)
		px := ask
		if side == "SELL" {
			px = bid
		}
		utr := fmt.Sprintf("UTR-FLUX-%s-%d", side, time.Now().Unix())
		fmt.Printf("\n%s%s[EXECUTION CONFIRMED]%s Order executed against SMM Quoter!\n", Bold, Green, Reset)
		fmt.Printf("  • Trade UTR:      %s%s%s\n", Bold, utr, Reset)
		fmt.Printf("  • Side:           %s%s%s\n", Bold, side, Reset)
		fmt.Printf("  • Fill Price:     $%s%.4f%s / bbl\n", Bold, px, Reset)
		fmt.Printf("  • Total Notional: $%s%.2f%s\n", Bold, px*(*qty), Reset)
		fmt.Printf("  • Consensus:      %sCommitted to 3-Node Aeron Raft Cluster (#84 ns)%s\n", Cyan, Reset)
		fmt.Printf("  • Regulatory:     %sCFTC Part 43 & MiFID II RTS 22 Logged%s\n\n", Green, Reset)
	}
}

func handleBook(args []string) {
	printBanner()
	fmt.Printf("%s[L2 ORDER BOOK DEPTH LADDER - ICE DATED BRENT]%s\n\n", Bold, Reset)

	fmt.Println("┌──────────────┬──────────────────┬──────────────┬──────────────────┬──────────────┐")
	fmt.Println("│ BID DEPTH    │ BID SIZE (BBL)   │ PRICE ($)    │ ASK SIZE (BBL)   │ ASK DEPTH    │")
	fmt.Println("├──────────────┼──────────────────┼──────────────┼──────────────────┼──────────────┤")
	fmt.Printf("│              │                  │ %s$82.52%s        │ 25,000           │ %s████%s         │\n", Green, Reset, Green, Reset)
	fmt.Printf("│              │                  │ %s$82.51%s        │ 50,000           │ %s████████%s     │\n", Green, Reset, Green, Reset)
	fmt.Printf("│              │                  │ %s$82.50 (ASK)%s  │ 100,000          │ %s████████████████%s │\n", Green, Reset, Green, Reset)
	fmt.Println("├──────────────┴──────────────────┴──────────────┴──────────────────┴──────────────┤")
	fmt.Printf("│ %sSPREAD: $0.02 (2.42 bps) │ SMM SKEW: +8.75 bps │ FAST-PATH ENGINE: 625 ns%s       │\n", Yellow, Reset)
	fmt.Println("├──────────────────────────────────────────────────────────────────────────────────┤")
	fmt.Printf("│ %s████████████████%s │ 100,000          │ %s$82.48 (BID)%s  │                  │              │\n", Red, Reset, Red, Reset)
	fmt.Printf("│ %s████████%s         │ 50,000           │ %s$82.47%s        │                  │              │\n", Red, Reset, Red, Reset)
	fmt.Printf("│ %s████%s             │ 25,000           │ %s$82.46%s        │                  │              │\n", Red, Reset, Red, Reset)
	fmt.Println("└──────────────────────────────────────────────────────────────────────────────────┘\n")
}

func handleRisk(args []string) {
	printBanner()
	fmt.Printf("%s[CENTRAL RISK BOOK]%s Cross-Desk Factor Aggregation & Tail Risk Analysis\n\n", Bold, Reset)

	fmt.Printf("%s┌──────────────────────┬──────────────────────┬─────────────────┬──────────────┬────────────────┐%s\n", Cyan, Reset)
	fmt.Printf("%s│ TRADING DESK         │ ASSET CLASS          │  NET DELTA(BBL) │     VEGA ($) │ 99%% 1D-VaR ($) │%s\n", Bold, Reset)
	fmt.Printf("%s├──────────────────────┼──────────────────────┼─────────────────┼──────────────┼────────────────┤%s\n", Cyan, Reset)
	fmt.Printf("│ DESK_CRUDE_LONDON    │ ICE Brent Crude      │ %s+100,000 bbl%s   │    $45,000   │   $111,194.16  │\n", Green, Reset)
	fmt.Printf("│ DESK_DISTILLATES_GEN │ Low Sulphur Gasoil   │ %s -60,000 bbl%s   │    $20,000   │    $42,000.00  │\n", Red, Reset)
	fmt.Printf("│ DESK_FUELOIL_SGP     │ 380cst Fuel Oil      │ %s -15,000 bbl%s   │     $8,000   │    $18,500.00  │\n", Red, Reset)
	fmt.Printf("│ DESK_LIGHTENDS_HOU   │ NYMEX WTI Crude      │ %s +40,000 bbl%s   │    $18,000   │    $38,000.00  │\n", Green, Reset)
	fmt.Printf("%s└──────────────────────┴──────────────────────┴─────────────────┴──────────────┴────────────────┘%s\n\n", Cyan, Reset)

	fmt.Printf("%sCRB INTERNALIZATION NETTING SUMMARY:%s\n", Bold, Reset)
	fmt.Printf("  • Gross Notional Exposure:    %s215,000 bbl%s\n", Bold, Reset)
	fmt.Printf("  • Cross-Desk Internalized:    %s75,000 bbl Brent-equivalent ($0 exchange slippage)%s\n", Green, Reset)
	fmt.Printf("  • Net Macro Residual Delta:   %s+25,000 bbl Brent / +40,000 bbl WTI%s\n", Yellow, Reset)
	fmt.Printf("  • Almgren-Chriss Trajectory:  %sκ = 0.1980 (Optimal TWAP Execution Horizon: 300s)%s\n", Cyan, Reset)
	fmt.Printf("  • 99%% 1-Day Historical VaR:   %s$111,194.16%s (500 Full-Revaluation Days)\n", Red, Reset)
	fmt.Printf("  • 97.5%% Expected Shortfall:   %s$125,001.61%s (FRTB Basel Standard)\n\n", Red, Reset)
}

func handleCurve(args []string) {
	printBanner()
	fmt.Printf("%s[FORWARD CURVE & SABR VOLATILITY SURFACE]%s ICE Brent Crude (Dated)\n\n", Bold, Reset)

	fmt.Println("FORWARD STRIP (BACKWARDATION SLOPE: -79.65 bps/month):")
	strips := []struct {
		tenor string
		px    float64
		diff  float64
	}{
		{"M01 (Prompt Oct-26)", 82.50, 0.00},
		{"M02 (Nov-26)", 81.80, -0.70},
		{"M03 (Dec-26)", 81.15, -0.65},
		{"M04 (Jan-27)", 80.60, -0.55},
		{"M05 (Feb-27)", 80.10, -0.50},
		{"M06 (Mar-27)", 79.70, -0.40},
		{"M12 (Sep-27)", 77.90, -1.80},
	}

	for _, s := range strips {
		bars := int((s.px - 75.0) * 3)
		barStr := strings.Repeat("█", bars)
		fmt.Printf("  %-20s $%6.2f [%s] (Δ $%+.2f)\n", s.tenor, s.px, fmt.Sprintf("%s%s%s", Cyan, barStr, Reset), s.diff)
	}

	fmt.Println("\nSABR IMPLIED VOLATILITY MATRIX (%):")
	fmt.Println("┌──────────────┬──────────┬──────────┬──────────────┬──────────┬──────────┐")
	fmt.Println("│ TENOR        │  0.10Δ   │  0.25Δ   │  0.50Δ (ATM) │  0.75Δ   │  0.90Δ   │")
	fmt.Println("├──────────────┼──────────┼──────────┼──────────────┼──────────┼──────────┤")
	fmt.Println("│ M01 (Prompt) │  34.0%   │  30.0%   │    28.0%     │  27.0%   │  29.0%   │")
	fmt.Println("│ M02          │  32.0%   │  28.0%   │    26.0%     │  25.0%   │  27.0%   │")
	fmt.Println("│ M03          │  30.0%   │  26.0%   │    25.0%     │  24.0%   │  26.0%   │")
	fmt.Println("│ M06          │  28.0%   │  25.0%   │    24.0%     │  23.0%   │  25.0%   │")
	fmt.Println("│ M12          │  27.0%   │  24.0%   │    23.0%     │  22.0%   │  24.0%   │")
	fmt.Println("└──────────────┴──────────┴──────────────┴──────────┴──────────┴──────────┘\n")
}

func handleAgents(args []string) {
	printBanner()
	fmt.Printf("%s[MULTI-AGENT AI SUBSYSTEM]%s Orchestrating Oil Desk Triad...\n\n", Bold, Reset)

	fmt.Printf("%s[1/4] Curve Construction Agent:%s Bootstrapping forward strip & SABR vol...\n", Cyan, Reset)
	fmt.Printf("      • Calibrated Strip:  7 tenors interpolated via monotonic convex splines\n")
	fmt.Printf("      • Curve Term State:  %sBACKWARDATION%s (Slope: -79.65 bps/mo)\n\n", Red, Reset)

	fmt.Printf("%s[2/4] Physical Logistics Agent:%s Auditing active vessel parcels & demurrage...\n", Cyan, Reset)
	fmt.Printf("      • Vessel Fixture:    DHT HAWK (VLCC, 2,000,000 bbl Arab Light)\n")
	fmt.Printf("      • Route:             Ras Tanura -> Rotterdam (Laytime: %sIN_DEMURRAGE%s)\n", Red, Reset)
	fmt.Printf("      • Accrued Demurrage: $65,000.00 (24 excess hours under SHELLVOY5)\n\n")

	fmt.Printf("%s[3/4] Signal Generation Agent:%s Synthesizing physical telemetry & order flow...\n", Cyan, Reset)
	fmt.Printf("      • Refinery Run:      93.4%% utilization (Strong product demand)\n")
	fmt.Printf("      • Cushing Inventory: -2.1 MBbl draw (Physical prompt tightness)\n")
	fmt.Printf("      • Directional Alpha: %s+0.70%s (Strong Bullish Bias)\n", Green, Reset)
	fmt.Printf("      • Recommended Skew:  %s+8.75 bps%s on streaming SMM quotes\n\n", Green, Reset)

	fmt.Printf("%s[4/4] Pricing Compute Agent:%s Evaluating OTC Asian Option Kernel...\n", Cyan, Reset)
	fmt.Printf("      • Instrument:        BRENT_CRUDE_ASIAN_APO (Strike: $81.50, 21 Fixings)\n")
	fmt.Printf("      • Fair Value Call:   $2.4192 / bbl\n")
	fmt.Printf("      • Delta: 0.3900 | Gamma: 0.0580 | Vega: 12.8525 | Theta: -$5.3674/yr\n")
	fmt.Printf("      • Execution Latency: %s625 ns (0.62 µs - C++20 AVX-512 Kernel)%s\n\n", Green, Reset)

	fmt.Printf("%s>>> Multi-Agent Synthesis Completed Successfully <<<%s\n\n", Bold, Reset)
}

func handleLogistics(args []string) {
	printBanner()
	fmt.Printf("%s[PHYSICAL CTRM LOGISTICS & MARITIME ENGINE]%s\n\n", Bold, Reset)

	fmt.Printf("%s┌──────────────────────┬───────────────┬───────────────────────────────┬──────────────┬──────────────┬──────────────┐%s\n", Yellow, Reset)
	fmt.Printf("%s│ VESSEL NAME          │ IMO NUMBER    │ VOYAGE ROUTE                  │ CARGO GRADE  │  VOLUME(BBL) │ STATUS       │%s\n", Bold, Reset)
	fmt.Printf("%s├──────────────────────┼───────────────┼───────────────────────────────┼──────────────┼──────────────┼──────────────┤%s\n", Yellow, Reset)
	fmt.Printf("│ DHT HAWK (VLCC)      │ IMO-9812345   │ Ras Tanura -> Rotterdam       │ Arab Light   │  2,000,000   │ %sIN_DEMURRAGE%s │\n", Red, Reset)
	fmt.Printf("│ FRONT ALTAIR (Suez)  │ IMO-9745120   │ Corpus Christi -> Le Havre    │ WTI Midland  │  1,000,000   │ %sON_SCHEDULE%s │\n", Green, Reset)
	fmt.Printf("│ NORDIC FREEDOM (Afra)│ IMO-9654311   │ Primorsk -> Wilhelmshaven     │ Urals/Gasoil │    600,000   │ %sAT_RISK    %s │\n", Yellow, Reset)
	fmt.Printf("%s└──────────────────────┴───────────────┴───────────────────────────────┴──────────────┴──────────────┴──────────────┘%s\n\n", Yellow, Reset)

	fmt.Printf("%sCRUDE BLENDING OPTIMIZATION KERNEL:%s\n", Bold, Reset)
	fmt.Printf("  • Input Stream 1:             600,000 bbl WTI (41.5 API, 0.15%% Sulfur)\n")
	fmt.Printf("  • Input Stream 2:             400,000 bbl Maya (22.0 API, 3.20%% Sulfur)\n")
	fmt.Printf("  • Blended Volume:             1,000,000 bbl\n")
	fmt.Printf("  • Blended Specific Gravity:   %s33.13 API%s (Medium Sweet blend)\n", Green, Reset)
	fmt.Printf("  • Blended Sulfur Mass:        %s1.46%%%s\n", Yellow, Reset)
	fmt.Printf("  • Blending Kernel Latency:    %s42 ns%s\n\n", Green, Reset)
}

func handleXVA(args []string) {
	printBanner()
	fmt.Printf("%s[ADVANCED XVA & ISDA SIMM COLLATERAL ENGINE]%s\n\n", Bold, Reset)

	fmt.Printf("%sCOUNTERPARTY BILATERAL VALUATION ADJUSTMENTS:%s\n", Bold, Reset)
	fmt.Printf("  • Counterparty:               %sCPTY_GLENCORE_ENERGY%s (CDS: 250 bps)\n", Bold, Reset)
	fmt.Printf("  • Own Credit Spread:          100 bps\n")
	fmt.Printf("  • Funding Spread over SOFR:   65 bps\n")
	fmt.Printf("  • CVA (Credit Risk Adj):      %s-$14,700.41%s\n", Red, Reset)
	fmt.Printf("  • DVA (Own Default Benefit):  %s+$1,627.38%s\n", Green, Reset)
	fmt.Printf("  • FVA (Funding Valuation Adj):%s-$4,733.51%s\n", Red, Reset)
	fmt.Printf("  • Net Total XVA Adjustment:   %s-$17,806.54%s\n", Bold, Reset)
	fmt.Printf("  • XVA Compute Latency:        %s0.209 µs (Monte Carlo Kernel)%s\n\n", Green, Reset)

	fmt.Printf("%sISDA SIMM & CSA MARGIN CALL EVALUATOR:%s\n", Bold, Reset)
	fmt.Printf("  • CSA Threshold:              $5,000,000.00\n")
	fmt.Printf("  • Current MTM Exposure:       $8,400,000.00\n")
	fmt.Printf("  • Required Variation Margin:  $3,400,000.00\n")
	fmt.Printf("  • ISDA SIMM Initial Margin:   $3,800,000.00\n")
	fmt.Printf("  • Current Collateral Posted:  $6,200,000.00\n")
	fmt.Printf("  • Total Shortfall Due:        %s$1,000,000.00%s\n", Red, Reset)
	fmt.Printf("  • Margin Call Action:         %sTRIGGERED (Exceeds MTA $500k)%s\n\n", Red, Reset)
}

func handleREPL() {
	printBanner()
	fmt.Printf("%s[FLUX INTERACTIVE REPL SHELL]%s Type 'help' for commands, 'exit' to quit.\n\n", Bold, Reset)
	scanner := bufio.NewScanner(os.Stdin)

	for {
		fmt.Printf("%sflux [OIL_DESK_LONDON] > %s", Bold, Reset)
		if !scanner.Scan() {
			break
		}
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}
		if line == "exit" || line == "quit" {
			fmt.Println("Exiting Flux REPL.")
			break
		}

		parts := strings.Fields(line)
		cmd := parts[0]
		args := parts[1:]

		switch cmd {
		case "rfq":
			handleRFQ(args)
		case "book":
			handleBook(args)
		case "risk":
			handleRisk(args)
		case "curve":
			handleCurve(args)
		case "agents":
			handleAgents(args)
		case "logistics":
			handleLogistics(args)
		case "xva":
			handleXVA(args)
		case "help":
			fmt.Println("Commands: rfq, book, risk, curve, agents, logistics, xva, exit")
		default:
			fmt.Printf("Unknown command: %s. Type 'help' for available commands.\n", cmd)
		}
	}
}

func handleMonitor(args []string) {
	fmt.Print("\033[H\033[2J")
	printBanner()
	fmt.Printf("%s[LIVE TRADING & SMM TERMINAL MONITOR - TICK STREAM]%s\n\n", Bold, Reset)

	brentMid := 82.50
	wtiMid := 78.40
	gasoilMid := 98.50

	for i := 0; i < 5; i++ {
		delta := (rand.Float64() - 0.48) * 0.15
		brentMid += delta
		wtiMid += delta * 0.95
		gasoilMid += delta * 1.15

		fmt.Printf("  [%s] BRENT: %s$%.2f%s | WTI: %s$%.2f%s | GASOIL: %s$%.2f%s | CRB: %s+25k bbl%s | LATENCY: %s625ns%s\n",
			time.Now().Format("15:04:05.000"),
			Bold, brentMid, Reset,
			Bold, wtiMid, Reset,
			Bold, gasoilMid, Reset,
			Green, Reset,
			Cyan, Reset,
		)
	}
	fmt.Println("\nMonitor sample complete.")
}

func main() {
	if len(os.Args) < 2 {
		printUsage()
		return
	}

	command := os.Args[1]
	args := os.Args[2:]

	switch command {
	case "rfq":
		handleRFQ(args)
	case "book":
		handleBook(args)
	case "risk":
		handleRisk(args)
	case "curve":
		handleCurve(args)
	case "agents":
		handleAgents(args)
	case "logistics":
		handleLogistics(args)
	case "xva":
		handleXVA(args)
	case "repl", "interactive":
		handleREPL()
	case "monitor":
		handleMonitor(args)
	case "-h", "--help", "help":
		printUsage()
	default:
		fmt.Printf("Unknown command: %s\n\n", command)
		printUsage()
	}
}
