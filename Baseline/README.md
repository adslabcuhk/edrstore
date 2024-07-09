# Baseline

Several baseline approaches of encrypted data reduction. We  compare EDRStore with those baseline approaches (see Exp#1 and Exp#3 in our  paper).

## Build

To compile the whole project:

```bash
$ cd ./Baseline
$ bash setup.sh # for the first build
```

If the complication is successful, the executable file is the `bin` folder:

```bash
# currently in the ./Baseline
$ ls ./bin
ClientMain* config.json Container KeyManager* Recipes ServerMain*
```

`config.json`: the configuration file

`Containers`: the folder to store the container files

`Recipes`: the folder to store the file recipes

`ClientMain`: the client

`ServerMain`: the storage server (cloud)

`KeyManager`: the dedicate key server

To re-compile the project and clean store data in the `bin` folder:

```bash
$ cd ./Baseline
$ bash recompile.sh # clean all store data
```

Note that `recompile.sh` will copy `./Baseline/config.json` to `./Baseline/bin/` for re-configuration. Please ensure that `config.json` in `./Baseline/bin` is correct in your configuration (You can edit the `./Baseline/config.json` to set up your configuration and run `recompile.sh` to validate your configuration).

## Usage

- Configuration: you can use config.json to configure the system

```json
{
    "Chunker": {
        "chunking_type": 1,
        "max_chunk_size": 16384,
        "avg_chunk_size": 8192,
        "min_chunk_size": 4096,
        "sliding_win_size": 128,
        "read_size": 128
    },
    "Similar": {
        "sliding_win_size": 48
    },
    "StorageServer": {
        "ip": "127.0.0.1",
        "port": 16666,
        "recipe_root_path": "Recipes/",
        "container_root_path": "Containers/",
        "fp_2_chunk_db": "fp_chunk_db",
        "feature_2_fp_db": "feature_fp_db",
        "container_cache_size": 512
    },
    "KeyServer": {
        "ip": "127.0.0.1",
        "port": 16667,
        "feature_2_key_db": "feature_key_db"
    },
    "Client": {
        "id": 1,
        "send_chunk_batch_size": 512,
        "send_recipe_batch_size": 1024,
        "user_key": "0123456789"
    }
}
```

Note that you need to modify `storageServerIp_`, and `storageServerPort_` according to the machines that run the storage server (cloud).

- Client usage:

Check the command specification:

```bash
$ cd ./Baseline/bin
$ ./ClientMain -h
-t: operation ([u/d]):
        u: upload
        d: download
-m [key generation method]:
        0: Plain
        1: Server-aided MLE
        2: Encrypted Local Compression
```

`-t`: operation type, upload/download

`-i`: input file path

- Storage server usage:

```bash
$ cd ./Baseline/bin
$ ./ServerMain
```

- Key manager usage:

```bash
$ cd ./Baseline/bin
$ ./KeyManager
```

- clean up

You can use "ctrl + C" to stop the storage server and key manager. 

