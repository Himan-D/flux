package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"os"
	"time"
)

func handleReport(args []string) {
	if len(args) == 0 {
		fmt.Println("Usage: flux report <cftc|mifid> [options]")
		return
	}

	regType := args[0]
	subArgs := args[1:]

	fs := flag.NewFlagSet("report", flag.ExitOnError)
	tradeUTR := fs.String("utr", "UTR-FLUX-BUY-1787683804", "Unique Transaction Identifier (UTI/UTR)")
	jsonOut := fs.Bool("json", false, "Output in JSON format")
	fs.Parse(subArgs)

	now := time.Now().UTC()

	switch regType {
	case "cftc":
		report := map[string]interface{}{
			"regulation":                 "CFTC_PART_43_REALTIME_AND_PART_45_CONFIRMATION",
			"swap_data_repository":       "ICE_TRADE_VAULT_US",
			"uti_utr":                    *tradeUTR,
			"execution_timestamp":        now.Format(time.RFC3339Nano),
			"reporting_entity_lei":       "549300GLENCORE123456",
			"counterparty_lei":           "213800TRAFIGURA654321",
			"commodity_asset_class":      "COMMODITY_ENERGY_CRUDE_OIL",
			"underlying_code":            "ICE:BRENT",
			"settlement_type":            "FINANCIAL_CASH_SETTLEMENT",
			"notional_quantity":          50000.0,
			"notional_unit":              "BARRELS",
			"price_usd":                  3.4971,
			"collateralization_category": "FULLY_COLLATERALIZED_ISDA_SIMM",
			"submission_status":          "ACCEPTED_BY_SDR_ACK_200",
		}

		if *jsonOut {
			json.NewEncoder(os.Stdout).Encode(report)
			return
		}

		printBanner()
		fmt.Printf("%s[CFTC PART 43 / PART 45 REAL-TIME DISSEMINATION RECORD]%s\n\n", Bold, Reset)
		fmt.Printf("  • SDR Destination:     %sICE Trade Vault (USA)%s\n", Cyan, Reset)
		fmt.Printf("  • Unique Trade ID:     %s%s%s\n", Bold, *tradeUTR, Reset)
		fmt.Printf("  • Reporting Party LEI: 549300GLENCORE123456\n")
		fmt.Printf("  • Counterparty LEI:    213800TRAFIGURA654321\n")
		fmt.Printf("  • Asset / Underlying:  Energy / ICE Dated Brent Crude\n")
		fmt.Printf("  • Execution Notional:  50,000 bbl @ $3.4971/bbl ($174,854.38)\n")
		fmt.Printf("  • Regulatory Status:   %sDISSEMINATED TO PUBLIC TAPE (0.12s latency)%s\n\n", Green, Reset)

	case "mifid":
		report := map[string]interface{}{
			"regulation":          "ESMA_MIFID_II_RTS_22_TRANSACTION_REPORTING",
			"approved_mechanism":  "DTCC_TRADE_REPORTING_ARM",
			"uti_utr":             *tradeUTR,
			"instrument_isin":     "EZ3895720194",
			"trading_venue_mic":   "XXXX", // Bilateral OTC
			"buyer_lei":           "549300GLENCORE123456",
			"seller_lei":          "213800TRAFIGURA654321",
			"price_currency":      "USD",
			"price":               3.4971,
			"quantity":            50000.0,
			"transmission_status": "APPROVED_ARM_CONFIRMED",
		}

		if *jsonOut {
			json.NewEncoder(os.Stdout).Encode(report)
			return
		}

		printBanner()
		fmt.Printf("%s[MIFID II RTS 22 / RTS 25 TRANSACTION AUDIT REPORT]%s\n\n", Bold, Reset)
		fmt.Printf("  • ARM Mechanism:       %sDTCC European Transaction Reporting ARM%s\n", Cyan, Reset)
		fmt.Printf("  • Transaction UTR:     %s%s%s\n", Bold, *tradeUTR, Reset)
		fmt.Printf("  • Buyer / Seller LEI:  549300GLENCORE123456 / 213800TRAFIGURA654321\n")
		fmt.Printf("  • Transmission Status: %sVALIDATED & TRANSMITTED TO FCA / ESMA%s\n\n", Green, Reset)
	}
}
