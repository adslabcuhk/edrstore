import os
prefix = "https://github.com/bminor/glibc/archive/refs/tags/"

version_set = [
    "glibc-1.09",
    "glibc-1.90",
    "glibc-1.91",
    "glibc-1.92",
    "glibc-1.93",
    "glibc-2.0.2",
    "glibc-2.0.4",
    "glibc-2.0.5",
    "glibc-2.0.5b",
    "glibc-2.0.6",
    "glibc-2.0.92",
    "glibc-2.0.95",
    "glibc-2.0.96",
    "glibc-2.0.97",
    "glibc-2.0.98",
    "glibc-2.0.99",
    "glibc-2.0.100",
    "glibc-2.0.101",
    "glibc-2.0.103",
    "glibc-2.0.106",
    "glibc-2.0.112",
    "glibc-2.1",
    "glibc-2.1.1",
    "glibc-2.1.2",
    "glibc-2.1.91",
    "glibc-2.1.92",
    "glibc-2.1.93",
    "glibc-2.1.94",
    "glibc-2.1.95",
    "glibc-2.1.96",
    "glibc-2.1.97",
    "glibc-2.2",
    "glibc-2.2.1",
    "glibc-2.2.2",
    "glibc-2.2.3",
    "glibc-2.2.4",
    "glibc-2.2.5",
    "glibc-2.3",
    "glibc-2.3.1",
    "glibc-2.3.2",
    "glibc-2.3.3",
    "glibc-2.3.4",
    "glibc-2.3.5",
    "glibc-2.3.6",
    "glibc-2.4",
    "glibc-2.5",
    "glibc-2.5.1",
    "glibc-2.6",
    "glibc-2.6.1",
    "glibc-2.7",
    "glibc-2.8",
    "glibc-2.9",
    "glibc-2.10",
    "glibc-2.10.1",
    "glibc-2.10.2",
    "glibc-2.11",
    "glibc-2.11.1",
    "glibc-2.11.2",
    "glibc-2.11.3",
    "glibc-2.12",
    "glibc-2.12.1",
    "glibc-2.12.2",
    "glibc-2.13",
    "glibc-2.14",
    "glibc-2.14.1",
    "glibc-2.15",
    "glibc-2.16",
    "glibc-2.16-ports-before-merge",
    "glibc-2.16-ports-merge",
    "glibc-2.16-tps",
    "glibc-2.16.0",
    "glibc-2.16.90",
    "glibc-2.17",
    "glibc-2.17.90",
    "glibc-2.18",
    "glibc-2.18.90",
    "glibc-2.19",
    "glibc-2.19.90",
    "glibc-2.20",
    "glibc-2.20.90",
    "glibc-2.21",
    "glibc-2.21.90",
    "glibc-2.22",
    "glibc-2.22.90",
    "glibc-2.23",
    "glibc-2.23.90",
    "glibc-2.24",
    "glibc-2.24.90",
    "glibc-2.25",
    "glibc-2.25.90",
    "glibc-2.26",
    "glibc-2.26.9000",
    "glibc-2.27",
    "glibc-2.27.9000",
    "glibc-2.28",
    "glibc-2.28.9000",
    "glibc-2.29",
    "glibc-2.29.9000",
    "glibc-2.30",
    "glibc-2.30.9000",
    "glibc-2.31",
    "glibc-2.31.9000",
    "glibc-2.32",
    "glibc-2.32.9000",
    "glibc-2.33",
    "glibc-2.33.9000",
    "glibc-2.34",
    "glibc-2.34.9000",
    "glibc-2.35",
    "glibc-2.35.9000"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/glibc_src/"

def Download(input_link):
    cmd = "wget --no-check-certificate -P " + download_output_folder + " " + input_link + ".tar.gz"
    print(cmd)
    os.system(cmd)

def Process(input_version):
    # uncompressed
    input_file = download_output_folder + input_version + ".tar.gz"
    cmd = "tar " + uncompressed_flag + " " + input_file + " -C " + download_output_folder
    print(cmd)
    os.system(cmd)
    # remove the download file
    cmd = "rm " + input_file
    print(cmd)
    os.system(cmd)

    # re-pack the folder
    repack_folder = download_output_folder + "glibc-" + input_version
#    output_file_name = repack_output_folder + input_version[0:4] + ".tar"
    output_file_name = repack_output_folder + input_version + ".tar"
    cmd = "tar " + package_flag + " " + output_file_name + " " + repack_folder
    print(cmd)
    os.system(cmd)
    cmd = "rm -rf " + repack_folder
    print(cmd)
    os.system(cmd)

if __name__ == "__main__":

    print version_set

    for version in version_set:
        input_link = prefix + version
        Download(input_link)
        Process(version)
