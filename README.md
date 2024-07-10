# Encrypted Data Reduction: Removing Redundancy from Encrypted Data in Outsourced Storage (EDRStore)

## Introduction

EDRStore is an outsourced storage system that supports deduplication, delta compression, and local compression for storage savings, and further combines them with encryption for data confidentiality. This repo contains the implementation of EDRStore prototype, baseline approaches, and scripts for trace collecting.

- `./EDRStore`: includes the implementation of EDRStore prototype.
- `./Baseline`: includes the implementation of all baseline approaches (e.g., MLE, ELC, and Plain).
- `./Script`: includes the scripts to collect traces used in evaluations in our paper.

Note that each folder has a separate README file to introduce the build instructions and usage.

## Publication

Jia Zhao, Zuoru Yang, Jingwei Li, and Patrick P. C. Lee. [Encrypted Data Reduction: Removing Redundancy from Encrypted Data in Outsourced Storage.](https://www.cse.cuhk.edu.hk/~pclee/www/pubs/tos24edrstore.pdf) Accepted for publication in ACM Transactions on Storage (TOS).

## Dependencies

- Basic packages: g++, cmake, openssl, libleveldb-dev, liblz4-dev, libzstd-dev, librocksdb-dev, libboost-all-dev, libsnappy-dev, libbz2-dev, and jemalloc

The packages above can be directly installed via `apt-get`:

`sudo apt-get install g++ cmake libssl-dev libleveldb-dev liblz4-dev libzstd-dev librocksdb-dev libboost-all-dev libsnappy-dev libbz2-dev  libjemalloc-dev`

Note that we require the version of OpenSSL should be at least **1.1.0**. If the default version of OpenSSL from `apt-get` is older than1.1.0, please install OpenSSL manually from this [link](https://www.openssl.org/source/).

## Build & Usage

Please refer to the README files in `./EDRStore` and`./Baseline`, and for the building instruction and usage of each component.

## Traces

We use six real-world datasets of backup workloads in our paper. We introduce how to collect traces as follows:

- TENSORFLOW

```bash
# download the TENSORFLOW trace
cd ./Script
python3 get_tensorflow_src.py
```

- DOCKER

```bash
# download the DOCKER trace
cd ./Script
python3 get_docker.py
```

- GCC

```bash
# download the GCC trace
cd ./Script
python3 get_gcc_source_code.py
```

- CHROM

```bash
# download the CHROM trace
cd ./Script
python3 get_chrome_source_code.py
```

- WEB

```bash
# this dataset was captured and used in previous work ([72, 78] in our paper).
```

- LINUX

```bash
# download the LINUX trace
cd ./Script
python3 get_linux_rc.py
```

## Getting Started Instructions

Here we provide some instructions to quickly check the effectiveness of each component.

- Preparation

Please ensure that you have successfully compiled three components based on their README files in each folder and prepare three machines (a client, a storage server, and a key server) connected by the network.

In the client machine, you need to configure the `ip` under `StorageServer` to the storage server machine IP in `config.json`. (e.g., "ip": "192.168.10.64"), and also the `ip` under `KeyServer` to the key server machine IP.

**Not that we recommend to edit the `config.json` in `./EDRStore` (or `./Baseline` for baseline approaches)  and run `recompile.sh` to enforce the configuration (it will automatically copy the `config.json` to `./EDRStore/bin` or `./Baseline/bin`).**

Please refer to the separate README files in `./EDRStore` (or `./Baseline` for baseline approaches) for usage instructions and parameter explainations.

- Question

If you have any questions, please feel free to contact `jzhao@cse.cuhk.edu.hk`