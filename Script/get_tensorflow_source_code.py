import os
prefix = "https://github.com/tensorflow/tensorflow/archive/refs/tags/"

version_set = [
    "0.6.0",
    "0.7.0",
    "0.7.1",
    "0.8.0",
    "0.8.0rc0",
    "0.9.0",
    "0.9.0rc0",
    "0.10.0",
    "0.10.0rc0",
    "0.11.0",
    "0.11.0rc0",
    "0.11.0rc1",
    "0.11.0rc2",
    "0.12.0",
    "1.0.0",
    "1.0.0-alpha",
    "1.0.0-rc0",
    "1.0.0-rc1",
    "1.0.0-rc2",
    "1.0.1",
    "1.1.0",
    "1.1.0-rc0",
    "1.1.0-rc1",
    "1.1.0-rc2",
    "1.2.0",
    "1.2.0-rc0",
    "1.2.0-rc1",
    "1.2.0-rc2",
    "1.2.1",
    "1.3.0",
    "1.3.0-rc0",
    "1.3.0-rc1",
    "1.3.0-rc2",
    "1.3.1",
    "1.4.0",
    "1.4.0-rc0",
    "1.4.0-rc1",
    "1.4.1",
    "1.5.0",
    "1.5.0-rc0",
    "1.5.0-rc1",
    "1.5.1",
    "1.6.0",
    "1.6.0-rc0",
    "1.6.0-rc1",
    "1.7.0",
    "1.7.0-rc0",
    "1.7.0-rc1",
    "1.7.1",
    "1.8.0",
    "1.8.0-rc0",
    "1.8.0-rc1",
    "1.9.0",
    "1.9.0-rc0",
    "1.9.0-rc1",
    "1.9.0-rc2",
    "1.10.0",
    "1.10.0-rc0",
    "1.10.0-rc1",
    "1.10.1",
    "1.11.0",
    "1.11.0-rc0",
    "1.11.0-rc1",
    "1.11.0-rc2",
    "1.12.0",
    "1.12.0-rc0",
    "1.12.0-rc1",
    "1.12.0-rc2",
    "1.12.1",
    "1.12.2",
    "1.12.3",
    "1.13.0-rc0",
    "1.13.0-rc1",
    "1.13.0-rc2",
    "1.13.1",
    "1.13.2",
    "1.14.0",
    "1.14.0-rc0",
    "1.14.0-rc1",
    "1.15.0",
    "1.15.0-rc0",
    "1.15.0-rc1",
    "1.15.0-rc2",
    "1.15.0-rc3",
    "1.15.2",
    "1.15.3",
    "1.15.4",
    "1.15.5",
    "2.0.0",
    "2.0.0-alpha0",
    "2.0.0-beta0",
    "2.0.0-beta1",
    "2.0.0-rc0",
    "2.0.0-rc1",
    "2.0.0-rc2",
    "2.0.1",
    "2.0.2",
    "2.0.3",
    "2.0.4",
    "2.1.0",
    "2.1.0-rc0",
    "2.1.0-rc1",
    "2.1.0-rc2",
    "2.1.1",
    "2.1.2",
    "2.1.3",
    "2.1.4",
    "2.2.0",
    "2.2.0-rc0",
    "2.2.0-rc1",
    "2.2.0-rc2",
    "2.2.0-rc3",
    "2.2.0-rc4",
    "2.2.1",
    "2.2.2",
    "2.2.3",
    "2.3.0",
    "2.3.0-rc0",
    "2.3.0-rc1",
    "2.3.0-rc2",
    "2.3.1",
    "2.3.2",
    "2.3.3",
    "2.3.4",
    "2.4.0",
    "2.4.0-rc0",
    "2.4.0-rc1",
    "2.4.0-rc2",
    "2.4.0-rc3",
    "2.4.0-rc4",
    "2.4.1",
    "2.4.2",
    "2.4.3",
    "2.4.4",
    "2.5.0",
    "2.5.0-rc0",
    "2.5.0-rc1",
    "2.5.0-rc2",
    "2.5.0-rc3",
    "2.5.1",
    "2.5.2",
    "2.5.3",
    "2.6.0",
    "2.6.0-rc0",
    "2.6.0-rc1",
    "2.6.0-rc2",
    "2.6.1",
    "2.6.2",
    "2.6.3",
    "2.7.0",
    "2.7.0-rc0",
    "2.7.0-rc1",
    "2.7.1",
    "2.8.0",
    "2.8.0-rc0",
    "2.8.0-rc1",
    "2.9.0-rc0",
    "2.9.0-rc1",
    "2.9.0-rc2"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/tensorflow_src/"

def Download(input_link):
    cmd = "wget --no-check-certificate -P " + download_output_folder + " " + input_link + ".tar.gz"
    print(cmd)
    os.system(cmd)

def Process(input_version):
    # uncompressed
    input_file = download_output_folder + "v" + input_version + ".tar.gz"
    cmd = "tar " + uncompressed_flag + " " + input_file + " -C " + download_output_folder
    print(cmd)
    os.system(cmd)
    # remove the download file
    cmd = "rm " + input_file
    print(cmd)
    os.system(cmd)

    # re-pack the folder
    repack_folder = download_output_folder + "tensorflow-" + input_version
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
        input_link = prefix + "v" + version
        Download(input_link)
        Process(version)
