import os
prefix = "https://github.com/emacs-mirror/emacs/archive/refs/tags/"
#gcc-11.2.0.tar.gz
#version_set = ["gcc-11.2.0"]
version_set = [
    "emacs-18.59",
    "emacs-19.34",
    "emacs-20.1",
    "emacs-20.2",
    "emacs-20.3",
    "emacs-20.4",
    "emacs-21.1",
    "emacs-21.2",
    "emacs-21.3",
    "emacs-22.1",
    "emacs-22.2",
    "emacs-22.3",
    "emacs-23.1",
    "emacs-23.2",
    "emacs-23.3",
    "emacs-23.4",
    "emacs-24.5-rc3-fixed",
    "emacs-24.0.96",
    "emacs-24.0.97",
    "emacs-24.1",
    "emacs-24.2",
    "emacs-24.2.90",
    "emacs-24.2.91",
    "emacs-24.2.92",
    "emacs-24.2.93",
    "emacs-24.3-rc1",
    "emacs-24.3",
    "emacs-24.3.90",
    "emacs-24.3.91",
    "emacs-24.3.92",
    "emacs-24.3.93",
    "emacs-24.3.94",
    "emacs-24.4-rc1",
    "emacs-24.4",
    "emacs-24.4.90",
    "emacs-24.4.91",
    "emacs-24.5-rc1",
    "emacs-24.5-rc2",
    "emacs-24.5-rc3",
    "emacs-24.5",
    "emacs-25.0.90",
    "emacs-25.0.91",
    "emacs-25.0.92",
    "emacs-25.0.93",
    "emacs-25.0.94",
    "emacs-25.0.95",
    "emacs-25.1-rc1",
    "emacs-25.1-rc2",
    "emacs-25.1",
    "emacs-25.1.90",
    "emacs-25.1.91",
    "emacs-25.2",
    "emacs-25.2-rc1",
    "emacs-25.2-rc2",
    "emacs-25.3",
    "emacs-26.0.90",
    "emacs-26.0.91",
    "emacs-26.1",
    "emacs-26.1-rc1",
    "emacs-26.1.90",
    "emacs-26.1.91",
    "emacs-26.1.92",
    "emacs-26.2",
    "emacs-26.2.90",
    "emacs-26.3",
    "emacs-26.3-rc1",
    "emacs-27.0.90",
    "emacs-27.0.91",
    "emacs-27.1-rc1",
    "emacs-27.1",
    "emacs-27.1-rc2",
    "emacs-27.1.90",
    "emacs-27.1.91",
    "emacs-27.2",
    "emacs-27.2-rc1",
    "emacs-27.2-rc2",
    "emacs-28.0.90",
    "emacs-28.0.91",
    "emacs-28.0.92",
    "emacs-28.1"
]
uncompressed_flag = "-xvzf"
package_flag = "-cvf"
download_output_folder = "./" # for output folder
repack_output_folder = "~/emacs_src/"

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
    repack_folder = download_output_folder + "emacs-" + input_version
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
