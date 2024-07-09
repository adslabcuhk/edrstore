import os
prefix = "https://github.com/scilab/scilab/archive/refs/tags/"
#gcc-11.2.0.tar.gz
#version_set = ["gcc-11.2.0"]
version_set = [
    "2.6",
    "2.7",
    "3.0-rc1",
    "3.0",
    "3.1-rc1",
    "3.1",
    "4.0-rc1",
    "4.0",
    "4.1",
    "4.1-rc1",
    "4.1.1",
    "4.1.2",
    "5.0-alpha-2",
    "5.0-alpha-3",
    "5.0-alpha-1",
    "5.0-beta-1",
    "5.0-beta-2",
    "5.0-beta-3",
    "5.0-beta-4",
    "5.0-rc1",
    "5.0",
    "5.0.1",
    "5.0.2",
    "5.0.3",
    "5.3.2",
    "5.1",
    "5.1.1",
    "5.2.0",
    "5.2.0-beta-1",
    "5.2.1",
    "5.2.2",
    "5.3.0",
    "5.3.0-beta-1",
    "5.3.0-beta-2",
    "5.3.0-beta-3",
    "5.3.0-beta-4",
    "5.3.0-beta-5",
    "5.3.1",
    "5.3.3",
    "5.4.0",
    "5.4.0-alpha-1",
    "5.4.0-beta-1",
    "5.4.0-beta-2",
    "5.4.0-beta-3",
    "5.4.1",
    "5.5.0",
    "5.5.0-beta-1",
    "5.5.1",
    "5.5.2",
    "6.0.0",
    "6.0.0-alpha-1",
    "6.0.0-alpha-2",
    "6.0.0-beta-1",
    "6.0.0-beta-2",
    "6.0.1",
    "6.0.2",
    "6.1.0",
    "6.1.1"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/sciLab_src/"

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
    repack_folder = download_output_folder + "scilab-" + input_version
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
