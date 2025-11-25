# TUL-Internship: LZW Compression Acceleration Project

This repository contains the code developed during the **TUL-Internship** project, which focuses on the implementation and acceleration of the **Lempel-Ziv-Welch (LZW)** compression algorithm.

The project explores hardware acceleration via **High-Level Synthesis (HLS)** on different FPGA boards, along with the software orchestration required for parallelism.

---

## Repository Structure

The repository is organized around three main directories:

### 1. `Sw_Src_Codes` (Software References)

This folder holds the source codes for the **purely software implementations** of LZW **compression** and **decompression**. These codes serve as the **functional reference** for validation and performance comparison against the hardware accelerators.

### 2. `HLS_src_codes` (Hardware IP Cores)

This directory contains the C/C++ source code intended for High-Level Synthesis (HLS), defining the architecture of the custom IP Cores.

* **`Kria KV260 Version`**
    * Contains only the HLS code for the **Hash Version** of the LZW algorithm.

* **`ZedBoard`**
    * Contains the HLS code for **two (2) versions** of the algorithm:
        * The **Hash Version**.
        * The **Parallel Compression** version using a **single IP Core**.

### 3. `User_level_application` (Tests and Orchestration)

This directory holds the user-level application code used to **test, validate, and orchestrate** the IP Cores and software code.

* **`Kria KV260 Version`**
    * Contains **a single user-level application code** for this board, used to test the single Hash Version IP Core.

* **`ZedBoard`**
    * Contains the user-level applications required to test the **three performance scenarios**:
        1.  Test code for the **Hash Version**.
        2.  Test code for the **Parallel Compression (Single IP)**.
        3.  Orchestration code for the **Parallel Compression using Multiple IP Cores** (where parallelism is managed by the host code, utilizing multiple IP instances).

---

## For More Details

For a **more detailed explanation** of the architectural choices, HLS optimizations, and the full performance analysis of this internship project, please refer to the accompanying **Internship Report**.
