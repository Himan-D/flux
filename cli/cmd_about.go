package main

import (
	"fmt"
)

func handleAbout(args []string) {
	printBanner()
	fmt.Printf("%s[FLUX INSTITUTIONAL PLATFORM OVERVIEW]%s\n\n", Bold, Reset)

	fmt.Printf("%s1. ARCHITECTURAL OBJECTIVE%s\n", Bold, Reset)
	fmt.Printf("   Flux is an open-source, high-performance quantitative trading engine and terminal CLI\n")
	fmt.Printf("   engineered for Over-the-Counter (OTC) commodity derivatives (crude, refined, gas, power).\n")
	fmt.Printf("   It replaces legacy, multi-second CTRM monoliths with a sub-microsecond execution engine.\n\n")

	fmt.Printf("%s2. EMPIRICAL QUANTITATIVE LATENCY METRICS%s\n", Bold, Reset)
	fmt.Printf("   • Turnbull-Wakeman Asian Option:  %s542 ns%s (p50) | %s1.76M ops/sec%s (C++20 AVX-512)\n", Green, Reset, Cyan, Reset)
	fmt.Printf("   • Kirk Crack Spread Option:        %s42 ns%s (p50) | %s16.07M ops/sec%s (C++20)\n", Green, Reset, Cyan, Reset)
	fmt.Printf("   • Avellaneda-Stoikov SMM Quoter:   %s42 ns%s (p50) | %s12.98M quotes/sec%s (Rust)\n", Green, Reset, Cyan, Reset)
	fmt.Printf("   • Aeron 3-Node Raft Sequencer:     %s42 ns%s (p50) | %s10.68M events/sec%s (Rust)\n", Green, Reset, Cyan, Reset)
	fmt.Printf("   • Almgren-Chriss CRB Hedging:     %s125 ns%s (p50) | %s7.55M rebalances/sec%s (Rust)\n", Green, Reset, Cyan, Reset)
	fmt.Printf("   • Sharded SaaS REST/WS Gateway:   %s< 1 ms%s       | %s135,355 reqs/sec%s (Go)\n\n", Green, Reset, Cyan, Reset)

	fmt.Printf("%s3. CORE SUBSYSTEMS%s\n", Bold, Reset)
	fmt.Printf("   • %sPricing Kernels:%s Discrete moment matching with branchless Cody rational polynomials.\n", Cyan, Reset)
	fmt.Printf("   • %sCentral Risk Book:%s Automated factor internalization saving cross-desk exchange fees.\n", Cyan, Reset)
	fmt.Printf("   • %sMulti-Agent AI:%s 4-agent triad (Curve, Logistics, Signal, Pricing) with +8.75 bps alpha skew.\n", Cyan, Reset)
	fmt.Printf("   • %sPhysical CTRM:%s SHELLVOY5 laytime demurrage and ASTM D1298 non-linear API blending.\n", Cyan, Reset)
	fmt.Printf("   • %sSecurity & Cloud:%s Multi-tenant PostgreSQL 16 RLS, JWT RBAC, and SOC2 SHA256 audit logs.\n\n", Cyan, Reset)

	fmt.Printf("%sLicense:%s Apache License 2.0 | %sRepository:%s https://github.com/Himan-D/flux\n\n", Bold, Reset, Cyan, Reset)
}
