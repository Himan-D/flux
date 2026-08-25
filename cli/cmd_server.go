package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"net/http"
	"os"
	"os/exec"
	"time"
)

func handleServer(args []string) {
	if len(args) == 0 {
		fmt.Println("Usage: flux server <status|start|health> [options]")
		return
	}

	subCmd := args[0]
	subArgs := args[1:]

	switch subCmd {
	case "status", "health":
		cfg := loadConfig()
		url := fmt.Sprintf("%s/v1/health", cfg.APIGatewayURL)

		client := http.Client{Timeout: 2 * time.Second}
		resp, err := client.Get(url)
		if err != nil {
			printBanner()
			fmt.Printf("%s[FLUX SAAS CONTROL PLANE STATUS]%s\n", Bold, Reset)
			fmt.Printf("  • Server URL: %s\n", cfg.APIGatewayURL)
			fmt.Printf("  • Status:     %sOFFLINE / UNREACHABLE%s\n", Red, Reset)
			fmt.Printf("  • Run '%sflux server start%s' to launch local control plane.\n\n", Bold, Reset)
			return
		}
		defer resp.Body.Close()

		var data map[string]interface{}
		json.NewDecoder(resp.Body).Decode(&data)

		printBanner()
		fmt.Printf("%s[FLUX SAAS CONTROL PLANE STATUS]%s\n\n", Bold, Reset)
		fmt.Printf("  • Server URL:   %s\n", cfg.APIGatewayURL)
		fmt.Printf("  • HTTP Status:  %s200 OK (%s)%s\n", Green, data["status"], Reset)
		fmt.Printf("  • Service Name: %s\n", data["service"])
		fmt.Printf("  • Engine Mode:  %s%s%s\n\n", Green, data["engine_mode"], Reset)

	case "start":
		fs := flag.NewFlagSet("server start", flag.ExitOnError)
		port := fs.Int("port", 8080, "HTTP listen port")
		fs.Parse(subArgs)

		printBanner()
		fmt.Printf("%s[LAUNCHING FLUX CONTROL PLANE GATEWAY ON PORT %d]...%s\n", Bold, *port, Reset)
		
		cmd := exec.Command("go", "run", "saas-control/main.go")
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		if err := cmd.Start(); err != nil {
			fmt.Printf("Failed to start server: %v\n", err)
			return
		}
		fmt.Printf("%sServer running with PID %d on :%d%s\n\n", Green, cmd.Process.Pid, *port, Reset)
	}
}
