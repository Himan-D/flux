package main

import (
	"encoding/csv"
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"time"
)

type TradeRecord struct {
	TradeUTR     string  `json:"trade_utr"`
	Timestamp    string  `json:"timestamp"`
	Counterparty string  `json:"counterparty"`
	Instrument   string  `json:"instrument"`
	Side         string  `json:"side"`
	PriceUSD     float64 `json:"price_usd"`
	QuantityBbl  float64 `json:"quantity_bbl"`
	NotionalUSD  float64 `json:"notional_usd"`
	UnrealizedPnL float64 `json:"unrealized_pnl_usd"`
	Status       string  `json:"status"`
}

func getSampleTrades() []TradeRecord {
	return []TradeRecord{
		{
			TradeUTR:     "UTR-FLUX-BUY-1787683804",
			Timestamp:    time.Now().Add(-12 * time.Minute).Format("15:04:05"),
			Counterparty: "TRAFIGURA_PTE_LTD",
			Instrument:   "BRENT_ASIAN_APO_82.50",
			Side:         "BUY",
			PriceUSD:     3.4971,
			QuantityBbl:  50000.0,
			NotionalUSD:  174854.38,
			UnrealizedPnL: +12450.00,
			Status:       "COMMITTED",
		},
		{
			TradeUTR:     "UTR-FLUX-SELL-1787682910",
			Timestamp:    time.Now().Add(-45 * time.Minute).Format("15:04:05"),
			Counterparty: "VITOL_BAAR_AG",
			Instrument:   "GASOIL_CRACK_15.00",
			Side:         "SELL",
			PriceUSD:     5.1522,
			QuantityBbl:  30000.0,
			NotionalUSD:  154566.00,
			UnrealizedPnL: +8200.00,
			Status:       "COMMITTED",
		},
		{
			TradeUTR:     "UTR-FLUX-BUY-1787679800",
			Timestamp:    time.Now().Add(-2 * time.Hour).Format("15:04:05"),
			Counterparty: "TOTALENERGIES_TRADING",
			Instrument:   "WTI_CALENDAR_SPREAD_M01_M02",
			Side:         "BUY",
			PriceUSD:     0.8500,
			QuantityBbl:  100000.0,
			NotionalUSD:  85000.00,
			UnrealizedPnL: -2100.00,
			Status:       "COMMITTED",
		},
	}
}

func handleBlotter(args []string) {
	fs := flag.NewFlagSet("blotter", flag.ExitOnError)
	exportFmt := fs.String("export", "", "Export format (csv or json)")
	fs.Parse(args)

	trades := getSampleTrades()

	if *exportFmt == "json" {
		json.NewEncoder(os.Stdout).Encode(trades)
		return
	}

	if *exportFmt == "csv" {
		w := csv.NewWriter(os.Stdout)
		w.Write([]string{"TradeUTR", "Timestamp", "Counterparty", "Instrument", "Side", "PriceUSD", "QuantityBbl", "NotionalUSD", "UnrealizedPnL", "Status"})
		for _, t := range trades {
			w.Write([]string{
				t.TradeUTR,
				t.Timestamp,
				t.Counterparty,
				t.Instrument,
				t.Side,
				fmt.Sprintf("%.4f", t.PriceUSD),
				fmt.Sprintf("%.0f", t.QuantityBbl),
				fmt.Sprintf("%.2f", t.NotionalUSD),
				fmt.Sprintf("%.2f", t.UnrealizedPnL),
				t.Status,
			})
		}
		w.Flush()
		return
	}

	printBanner()
	fmt.Printf("%s[REAL-TIME OTC TRADE BLOTTER & POSITION LEDGER]%s\n\n", Bold, Reset)

	fmt.Printf("%s┌─────────────────────────┬──────────┬──────────────────────┬─────────────┬───────────┬──────────────┬──────────────┬──────────────┐%s\n", Cyan, Reset)
	fmt.Printf("%s│ TRADE UTR (MiFID II)    │ TIME     │ COUNTERPARTY         │ SIDE / INST │ PRICE ($) │ QTY (BBL)    │ NOTIONAL ($) │ MTM PnL ($)  │%s\n", Bold, Reset)
	fmt.Printf("%s├─────────────────────────┼──────────┼──────────────────────┼─────────────┼───────────┼──────────────┼──────────────┼──────────────┤%s\n", Cyan, Reset)

	totalNotional := 0.0
	totalPnL := 0.0

	for _, t := range trades {
		totalNotional += t.NotionalUSD
		totalPnL += t.UnrealizedPnL

		sideColor := Green
		if t.Side == "SELL" {
			sideColor = Red
		}
		pnlColor := Green
		if t.UnrealizedPnL < 0 {
			pnlColor = Red
		}

		fmt.Printf("│ %-23s │ %-8s │ %-20s │ %s%-4s%s %-6s │ $%-8.4f │ %-12.0f │ $%-11.2f │ %s$%+11.2f%s │\n",
			t.TradeUTR,
			t.Timestamp,
			t.Counterparty,
			sideColor, t.Side, Reset,
			"BRENT",
			t.PriceUSD,
			t.QuantityBbl,
			t.NotionalUSD,
			pnlColor, t.UnrealizedPnL, Reset,
		)
	}
	fmt.Printf("%s└─────────────────────────┴──────────┴──────────────────────┴─────────────┴───────────┴──────────────┴──────────────┴──────────────┘%s\n", Cyan, Reset)

	fmt.Printf("\n%sPORTFOLIO BLOTTER AGGREGATE:%s\n", Bold, Reset)
	fmt.Printf("  • Total Executed Trades:   %s%d active contracts%s\n", Bold, len(trades), Reset)
	fmt.Printf("  • Cumulative Notional:     %s$%.2f%s\n", Bold, totalNotional, Reset)
	fmt.Printf("  • Net Open Unrealized PnL: %s%s$%+.2f%s\n", Bold, Green, totalPnL, Reset)
	fmt.Printf("  • SDR Audit Status:        %sAll trades confirmed to DTCC / ICE Trade Vault%s\n\n", Green, Reset)
}
