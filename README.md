<img src="https://github.com/onlyone-rnd/UltraISO_KeyGen/blob/main/images/amd.png" width="400" /> <img src="https://github.com/onlyone-rnd/UltraISO_KeyGen/blob/main/images/geforce.png" width="400" />

# UltraISO KeyGen (OpenCL Edition)

> ⚠️ **Concept / Proof-of-Concept only**

> Initial idea - https://github.com/dagangwood163/PoCUltraISOKeyGen

> This project demonstrates using **MD5 collision search** and **GPU acceleration** to generate a registration code that matches a pre-defined MD5 pattern.
> It is **not** intended or permitted for use against commercial software, license systems or any third-party products.

---

## Key Ideas

- **MD5 as license check.**
  Instead of a classic “serial → deterministic formula check”, the verification is done via MD5.  
  The keygen reproduces this logic externally, brute forcing candidates until one matches the required MD5 pattern.

  - **Collision search on a whitelist.**
  To checking, **whitelist** of allowed MD5 signatures is used.

  - **GPU acceleration (OpenCL).**
  The heavy work — generating and testing large numbers of candidates — is offloaded onto the GPU:
  - the OpenCL kernel performs modular arithmetic and MD5 for each candidate;
  - results are sent back to the CPU for final filtering against the whitelist.

  - **CPU acceleration (multithreading).**
  To speed up generating, you can enable CPU multithreading. (uncheck "GPU only")  

  - **CPU as coordinator.**
  The CPU:
  - prepares candidate batches;
  - launches the OpenCL kernel;
  - performs a fast lookup in the whitelist;
  - stops computation as soon as a valid code is found.

---

## Features

- MD5 collision-style search against a predefined pattern (e.g. partial 48-bit match).
- Hybrid CPU/GPU pipeline.
- GUI frontend.

---

## Requirements

- **OS:** Windows 10/11 x64  
- **Compiler:** Visual Studio 2022 (MSVC, x64)
- **GPU / Drivers:**
- GPU with **OpenCL 1.2+** support (AMD / NVIDIA / Intel);

---

## Usage

> ⚠️ **For demonstration and test data only.**  
> Do not use this project to bypass licensing or protection of commercial software.

1. Run the compiled executable.
2. Enter a test name / identifier.
3. Start the generation:
   - logs will show GPU/CPU usage and throughput (candidates per second);
   - once a matching candidate is found, the corresponding registration code is displayed.

---

## Legal & Ethical Notice

This project is provided strictly for **research and educational purposes**:

- it does **not** contain data or logic tied to any specific commercial product;
- it must **not** be used to circumvent licensing or protection of third-party software;
- the author of the code assumes no responsibility for any misuse.

If you use ideas or code from this project, do so only in contexts where it is clearly legal:

- bug bounty / reverse-engineering tasks with explicit permission;
- your own software and licensing schemes;
- academic / research work where such use is clearly permitted.

