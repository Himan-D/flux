package main

import (
	"encoding/json"
	"fmt"
	"os"
	"path/filepath"
)

type CLIConfig struct {
	ActiveTenantID   string `json:"active_tenant_id"`
	ActiveDeskID     string `json:"active_desk_id"`
	DefaultCurrency  string `json:"default_currency"`
	APIGatewayURL    string `json:"api_gateway_url"`
	AeronClusterHost string `json:"aeron_cluster_host"`
	AutoLogCFTC      bool   `json:"auto_log_cftc"`
}

func defaultConfig() CLIConfig {
	return CLIConfig{
		ActiveTenantID:   "TENANT_GLENCORE_ENERGY_LTD",
		ActiveDeskID:     "DESK_OIL_DERIVATIVES_LONDON",
		DefaultCurrency:  "USD",
		APIGatewayURL:    "http://localhost:8080",
		AeronClusterHost: "10.0.1.1:40123",
		AutoLogCFTC:      true,
	}
}

func getConfigPath() string {
	home, err := os.UserHomeDir()
	if err != nil {
		home = "."
	}
	return filepath.Join(home, ".flux", "config.json")
}

func loadConfig() CLIConfig {
	p := getConfigPath()
	data, err := os.ReadFile(p)
	if err != nil {
		return defaultConfig()
	}
	var cfg CLIConfig
	if err := json.Unmarshal(data, &cfg); err != nil {
		return defaultConfig()
	}
	return cfg
}

func saveConfig(cfg CLIConfig) error {
	p := getConfigPath()
	os.MkdirAll(filepath.Dir(p), 0755)
	data, err := json.MarshalIndent(cfg, "", "  ")
	if err != nil {
		return err
	}
	return os.WriteFile(p, data, 0644)
}

func handleConfig(args []string) {
	if len(args) == 0 || args[0] == "show" {
		cfg := loadConfig()
		printBanner()
		fmt.Printf("%s[FLUX ACTIVE CONFIGURATION]%s\n", Bold, Reset)
		fmt.Printf("  • Config Path:        %s\n", getConfigPath())
		fmt.Printf("  • Active Tenant:      %s%s%s\n", Cyan, cfg.ActiveTenantID, Reset)
		fmt.Printf("  • Active Desk:        %s%s%s\n", Cyan, cfg.ActiveDeskID, Reset)
		fmt.Printf("  • Default Currency:   %s\n", cfg.DefaultCurrency)
		fmt.Printf("  • API Gateway URL:    %s\n", cfg.APIGatewayURL)
		fmt.Printf("  • Aeron Cluster Host: %s\n", cfg.AeronClusterHost)
		fmt.Printf("  • Auto Log CFTC:      %t\n\n", cfg.AutoLogCFTC)
		return
	}

	if args[0] == "set" && len(args) >= 3 {
		cfg := loadConfig()
		key := args[1]
		val := args[2]

		switch key {
		case "tenant":
			cfg.ActiveTenantID = val
		case "desk":
			cfg.ActiveDeskID = val
		case "api":
			cfg.APIGatewayURL = val
		case "currency":
			cfg.DefaultCurrency = val
		default:
			fmt.Printf("Unknown config key: %s (supported: tenant, desk, api, currency)\n", key)
			return
		}

		if err := saveConfig(cfg); err != nil {
			fmt.Printf("Failed to save config: %v\n", err)
			return
		}
		fmt.Printf("%sConfig updated successfully!%s [%s = %s]\n", Green, Reset, key, val)
		return
	}

	fmt.Println("Usage:")
	fmt.Println("  flux config show")
	fmt.Println("  flux config set <tenant|desk|api|currency> <value>")
}
