import os
prefix = "https://github.com/gcc-mirror/gcc/archive/refs/tags/releases/"
#gcc-11.2.0.tar.gz
#version_set = ["gcc-11.2.0"]
version_set = ["gcc-11.2.0", "gcc-11.1.0", \
"gcc-10.3.0", "gcc-10.2.0", "gcc-10.1.0", \
"gcc-9.4.0", "gcc-9.3.0", "gcc-9.2.0", "gcc-9.1.0", \
"gcc-8.5.0", "gcc-8.4.0", "gcc-8.3.0", "gcc-8.2.0", "gcc-8.1.0", \
"gcc-7.5.0", "gcc-7.4.0", "gcc-7.3.0", "gcc-7.2.0", "gcc-7.1.0", \
 "gcc-6.5.0", "gcc-6.4.0", "gcc-6.3.0", "gcc-6.2.0", "gcc-6.1.0", \
 "gcc-5.5.0", "gcc-5.4.0", "gcc-5.3.0", "gcc-5.2.0", "gcc-5.1.0", \
 "gcc-4.9.4", "gcc-4.9.3", "gcc-4.9.2", "gcc-4.9.1", "gcc-4.9.0", \
 "gcc-4.8.5", "gcc-4.8.4", "gcc-4.8.3", "gcc-4.8.2", "gcc-4.8.1", "gcc-4.8.0", \
 "gcc-4.7.4", "gcc-4.7.3", "gcc-4.7.2", "gcc-4.7.1", "gcc-4.7.0", \
 "gcc-4.6.4", "gcc-4.6.3", "gcc-4.6.2", "gcc-4.6.1", "gcc-4.6.0", \
 "gcc-4.5.4", "gcc-4.5.3", "gcc-4.5.2", "gcc-4.5.1", "gcc-4.5.0", \
 "gcc-4.4.7", "gcc-4.4.6", "gcc-4.4.5", "gcc-4.4.4", "gcc-4.4.3", "gcc-4.4.2", "gcc-4.4.1", "gcc-4.4.0", \
 "gcc-4.3.6", "gcc-4.3.5", "gcc-4.3.4", "gcc-4.3.3", "gcc-4.3.2", "gcc-4.3.1", "gcc-4.3.0", \
 "gcc-4.2.4", "gcc-4.2.3", "gcc-4.2.2", "gcc-4.2.1", "gcc-4.2.0", \
 "gcc-4.1.2", "gcc-4.1.1", "gcc-4.1.0", \
 "gcc-4.0.4", "gcc-4.0.3", "gcc-4.0.2", "gcc-4.0.1", "gcc-4.0.0", \
 "gcc-3.4.6", "gcc-3.4.5", "gcc-3.4.4", "gcc-3.4.3", "gcc-3.4.2", "gcc-3.4.1", "gcc-3.4.0", \
 "gcc-3.3.6", "gcc-3.3.5", "gcc-3.3.4", "gcc-3.3.3", "gcc-3.3.2", "gcc-3.3.1", "gcc-3.3.0", \
 "gcc-3.2.3", "gcc-3.2.2", "gcc-3.2.1", "gcc-3.2.0", \
 "gcc-3.1.1", "gcc-3.1.0", \
 "gcc-3.0.4", "gcc-3.0.3", "gcc-3.0.2", "gcc-3.0.1", "gcc-3.0.0", \
 "gcc-2.95.3", "gcc-2.95.2.1", "gcc-2.95.2", "gcc-2.95.1", "gcc-2.95.0"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/gcc_src/"

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
    repack_folder = download_output_folder + "gcc-releases-" + input_version
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
